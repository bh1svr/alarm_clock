#include "fsl_debug_console.h"
#include "fsl_p3t1755.h"
#include "fsl_i3c.h"
#include "i3c.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

/*******************************************************************************
 * 滤波参数
 ******************************************************************************/
#define I3C_TEMP_REG        0x00U   /* P3T1755 Temperature寄存器指针 */
#define IIR_SHIFT           5U      /* alpha = 1/32，越大响应越快、越不平滑 */
#define IIR_FRAC_BITS       8       /* 定点小数位数：raw(1/16°C)再左移8位 → 分辨率1/4096°C */
/* 偏移量的定点形式：-1.5°C = -24个raw(1/16°C)，再换算到Q8 */
#define TEMP_OFFSET_Q   (-24L << IIR_FRAC_BITS)     /* = -6144 */
#define EXAMPLE_MASTER             I3C0
#define I3C_MASTER_CLOCK_FREQUENCY CLOCK_GetI3CFClkFreq()
#define SENSOR_SLAVE_ADDR          0x48U
#define I3C_TIME_OUT_INDEX 100000000U

#define SENSOR_ADDR 0x08U
#define CCC_RSTDAA  0x06U
#define CCC_SETDASA 0x87

#ifndef EXAMPLE_I2C_BAUDRATE
#define EXAMPLE_I2C_BAUDRATE 400000
#endif
#ifndef EXAMPLE_I3C_OD_BAUDRATE
#define EXAMPLE_I3C_OD_BAUDRATE 1500000
#endif
#ifndef EXAMPLE_I3C_PP_BAUDRATE
#define EXAMPLE_I3C_PP_BAUDRATE 4000000
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void i3c_master_callback(I3C_Type *base, i3c_master_handle_t *handle, status_t status, void *userData);
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile status_t g_completionStatus;
volatile bool g_masterCompletionFlag;
i3c_master_handle_t g_i3c_m_handle;
p3t1755_handle_t p3t1755Handle;

/*******************************************************************************
 * 滤波器状态
 ******************************************************************************/
static int32_t iir_state_q = 0;     /* IIR状态，Q8定点，单位 1/4096 °C */
static bool    iir_valid   = false;  /* 是否已有有效输出 */

static int16_t med_win[3];          /* 中值滤波窗口，单位 1/16 °C */
static uint8_t med_idx = 0;
static uint8_t med_cnt = 0;

const i3c_master_transfer_callback_t masterCallback = {
    .slave2Master = NULL, .ibiCallback = NULL, .transferComplete = i3c_master_callback};

/*******************************************************************************
 * Code
 ******************************************************************************/
static void i3c_master_callback(I3C_Type *base, i3c_master_handle_t *handle, status_t status, void *userData)
{
    if (status == kStatus_Success)
    {
        g_masterCompletionFlag = true;
    }

    g_completionStatus = status;
}

status_t I3C_WriteSensor(uint8_t deviceAddress, uint32_t regAddress, uint8_t *regData, size_t dataSize)
{
    status_t result                  = kStatus_Success;
    i3c_master_transfer_t masterXfer = {0};
    uint32_t timeout                 = 0U;

    masterXfer.slaveAddress   = deviceAddress;
    masterXfer.direction      = kI3C_Write;
    masterXfer.busType        = kI3C_TypeI3CSdr;
    masterXfer.subaddress     = regAddress;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = regData;
    masterXfer.dataSize       = dataSize;
    masterXfer.flags          = kI3C_TransferDefaultFlag;

    g_masterCompletionFlag = false;
    g_completionStatus     = kStatus_Success;
    result                 = I3C_MasterTransferNonBlocking(EXAMPLE_MASTER, &g_i3c_m_handle, &masterXfer);
    if (kStatus_Success != result)
    {
        return result;
    }

    while (!g_masterCompletionFlag)
    {
        timeout++;
        if ((g_completionStatus != kStatus_Success) || (timeout > I3C_TIME_OUT_INDEX))
        {
            break;
        }
    }

    if (timeout == I3C_TIME_OUT_INDEX)
    {
        result = kStatus_Timeout;
    }
    result = g_completionStatus;

    return result;
}

status_t I3C_ReadSensor(uint8_t deviceAddress, uint32_t regAddress, uint8_t *regData, size_t dataSize)
{
    status_t result                  = kStatus_Success;
    i3c_master_transfer_t masterXfer = {0};
    uint32_t timeout                 = 0U;

    masterXfer.slaveAddress   = deviceAddress;
    masterXfer.direction      = kI3C_Read;
    masterXfer.busType        = kI3C_TypeI3CSdr;
    masterXfer.subaddress     = regAddress;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = regData;
    masterXfer.dataSize       = dataSize;
    masterXfer.flags          = kI3C_TransferDefaultFlag;

    g_masterCompletionFlag = false;
    g_completionStatus     = kStatus_Success;
    result                 = I3C_MasterTransferNonBlocking(EXAMPLE_MASTER, &g_i3c_m_handle, &masterXfer);
    if (kStatus_Success != result)
    {
        return result;
    }

    while (!g_masterCompletionFlag)
    {
        timeout++;
        if ((g_completionStatus != kStatus_Success) || (timeout > I3C_TIME_OUT_INDEX))
        {
            break;
        }
    }

    if (timeout == I3C_TIME_OUT_INDEX)
    {
        result = kStatus_Timeout;
    }
    result = g_completionStatus;

    return result;
}

status_t p3t1755_set_dynamic_address(void)
{
    status_t result                  = kStatus_Success;
    i3c_master_transfer_t masterXfer = {0};
    uint8_t g_master_txBuff[1];

    /* Reset dynamic address. */
    g_master_txBuff[0]      = CCC_RSTDAA;
    masterXfer.slaveAddress = 0x7E;
    masterXfer.data         = g_master_txBuff;
    masterXfer.dataSize     = 1;
    masterXfer.direction    = kI3C_Write;
    masterXfer.busType      = kI3C_TypeI3CSdr;
    masterXfer.flags        = kI3C_TransferDefaultFlag;
    result                  = I3C_MasterTransferBlocking(EXAMPLE_MASTER, &masterXfer);
    if (result != kStatus_Success)
    {
        return result;
    }

    /* Assign dynmic address. */
    memset(&masterXfer, 0, sizeof(masterXfer));
    g_master_txBuff[0]      = CCC_SETDASA;
    masterXfer.slaveAddress = 0x7E;
    masterXfer.data         = g_master_txBuff;
    masterXfer.dataSize     = 1;
    masterXfer.direction    = kI3C_Write;
    masterXfer.busType      = kI3C_TypeI3CSdr;
    masterXfer.flags        = kI3C_TransferNoStopFlag;
    result                  = I3C_MasterTransferBlocking(EXAMPLE_MASTER, &masterXfer);
    if (result != kStatus_Success)
    {
        return result;
    }

    memset(&masterXfer, 0, sizeof(masterXfer));
    g_master_txBuff[0]      = SENSOR_ADDR << 1;
    masterXfer.slaveAddress = SENSOR_SLAVE_ADDR;
    masterXfer.data         = g_master_txBuff;
    masterXfer.dataSize     = 1;
    masterXfer.direction    = kI3C_Write;
    masterXfer.busType      = kI3C_TypeI3CSdr;
    masterXfer.flags        = kI3C_TransferDefaultFlag;
    return I3C_MasterTransferBlocking(EXAMPLE_MASTER, &masterXfer);
}

/*!
 * @brief Main function
 */
void init_i3c_p3t1755(void)
{
    status_t result = kStatus_Success;
    i3c_master_config_t masterConfig;
    p3t1755_config_t p3t1755Config;

    /* Attach peripheral clock */
    CLOCK_SetClockDiv(kCLOCK_DivI3C0_FCLK, 4U);
    CLOCK_AttachClk(kFRO_HF_DIV_to_I3C0FCLK);

    init_i3c_pins();

    I3C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Hz.i2cBaud          = EXAMPLE_I2C_BAUDRATE;
    masterConfig.baudRate_Hz.i3cPushPullBaud  = EXAMPLE_I3C_PP_BAUDRATE;
    masterConfig.baudRate_Hz.i3cOpenDrainBaud = EXAMPLE_I3C_OD_BAUDRATE;
    masterConfig.enableOpenDrainStop          = false;
    masterConfig.disableTimeout               = true;
    I3C_MasterInit(EXAMPLE_MASTER, &masterConfig, I3C_MASTER_CLOCK_FREQUENCY);

    /* Create I3C handle. */
    I3C_MasterTransferCreateHandle(EXAMPLE_MASTER, &g_i3c_m_handle, &masterCallback, NULL);

    result = p3t1755_set_dynamic_address();
    if (result != kStatus_Success)
    {
        PRINTF("\r\nP3T1755 set dynamic address failed.\r\n");
    }

    p3t1755Config.writeTransfer = I3C_WriteSensor;
    p3t1755Config.readTransfer  = I3C_ReadSensor;
    p3t1755Config.sensorAddress = SENSOR_ADDR;
    P3T1755_Init(&p3t1755Handle, &p3t1755Config);
}

/*******************************************************************************
 * ① 直接读温度寄存器，得到12位补码raw（整数，1/16 °C）
 *    不经过P3T1755驱动的float转换，全程无浮点
 ******************************************************************************/
static status_t p3t1755_read_raw(p3t1755_handle_t *handle, int16_t *raw)
{
    uint8_t data[2];
    uint16_t u;

    status_t result = P3T1755_ReadReg(handle, P3T1755_TEMPERATURE_REG, &data[0], 2);
    if (result != kStatus_Success)
    {
        return result;
    }

    u = ((uint16_t)data[0] << 8) | data[1];
    u >>= 4;                        /* 保留12位有效数据 */
    if (u & 0x0800U)                /* 负数：符号位扩展到16位 */
    {
        u |= 0xF000U;
    }
    *raw = (int16_t)u;              /* 单位 0.0625 °C */
    return kStatus_Success;
}

/*******************************************************************************
 * ② 3点中值：去掉脉冲型野值
 ******************************************************************************/
static int16_t median3(int16_t a, int16_t b, int16_t c)
{
    int16_t t;
    if (a > b) { t = a; a = b; b = t; }   /* 保证 a <= b */
    if (a > c) return a;                  /* c < a <= b → 中值 a */
    if (b > c) return c;                  /* a <= c <  b → 中值 c */
    return b;                             /* a <= b <= c → 中值 b */
}

/*******************************************************************************
 * ③ 整数一阶IIR：y += (x - y) / 32，定点运算
 ******************************************************************************/
static void iir_update(int16_t median_raw)
{
    int32_t xq = (int32_t)median_raw << IIR_FRAC_BITS;

    if (!iir_valid)
    {
        iir_state_q = xq;           /* 首拍直通，避免从0爬升 */
        iir_valid   = true;
        return;
    }
    iir_state_q += (xq - iir_state_q) / (int32_t)(1UL << IIR_SHIFT);
}

/*!
 * @brief 获取滤波后温度，单位0.1°C，已四舍五入
 * @return 例如 22.5°C 返回 225；无有效数据返回 0
 */
int16_t get_temp_x10(void)
{
    int32_t n;

    if (!iir_valid)
    {
        return 0;   /* 与原 get_temp() 未初始化时返回0的行为一致 */
    }

    /* Q8状态(1/4096°C) → 0.1°C：先乘10再除4096 */
    n = (iir_state_q + TEMP_OFFSET_Q) * 10;

    /* 四舍五入到最近的0.1°C，负温同样对称（与C的round()语义一致） */
    n = (n >= 0) ? ((n + 2048) / 4096)
                 : -((-n + 2048) / 4096);

    return (int16_t)n;
}

/*******************************************************************************
 * 周期调用：读一次raw → 入中值窗口 → 窗口满后送IIR
 ******************************************************************************/
void read_temp(void)
{
    int16_t raw;

    if (p3t1755_read_raw(&p3t1755Handle, &raw) != kStatus_Success)
    {
        /* 总线异常：复位滤波器与窗口后重初始化，防止旧状态污染新数据 */
        iir_valid = false;
        iir_state_q = 0;
        med_cnt = 0;
        med_idx = 0;
        init_i3c_p3t1755();
        return;
    }

    med_win[med_idx] = raw;
    med_idx = (uint8_t)((med_idx + 1U) % 3U);
    if (med_cnt < 3U)
    {
        med_cnt++;
    }

    if (med_cnt == 3U)              /* 窗口填满后才启动IIR，前两拍不输出新值 */
    {
        iir_update(median3(med_win[0], med_win[1], med_win[2]));
    }
}
