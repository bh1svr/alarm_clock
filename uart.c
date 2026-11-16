#include "gps_parser.h"
#include <string.h>
#include "uart.h"
#include "fsl_lpuart_edma.h"
#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
#include "fsl_dmamux.h"
#endif
#define NMEA_BUF_SIZE    1024U           /* 乒乓双缓冲，每块 ≥2 句最长报文 */
#define RX_IRQ_PRIORITY    2U
/*---------------- 状态 ----------------*/
static uint8_t s_rxDmaBuf[NMEA_BUF_SIZE];
static volatile uint8_t s_rxDmaFullPending = 0U;
volatile uint32_t g_rxRearmError = 0U;
volatile uint32_t g_rxStateError = 0U;
static lpuart_edma_handle_t g_lpuartEdmaHandle;
static edma_handle_t g_lpuartTxEdmaHandle;
static edma_handle_t g_lpuartRxEdmaHandle;

#define LPUART_TX_DMA_CHANNEL       0U
#define LPUART_RX_DMA_CHANNEL       1U
#define LPUART1_TX_EDMA_CHANNEL kDma0RequestLPUART1Tx
#define LPUART1_RX_EDMA_CHANNEL kDma0RequestLPUART1Rx

static void RingWrite(const uint8_t *p, uint32_t n);
static status_t RearmRx(void);
static void RxHandleFull(void);

static void RxHandleIdle(void)
{
    uint32_t count = 0U;
    status_t status;

    /* 暂停 UART 向 DMA 发出新的接收请求 */
    LPUART_EnableRxDMA(LPUART1, false);
    __DSB();

    /* DMA 满缓冲区事件优先处理 */
    if (s_rxDmaFullPending != 0U)
    {
        RxHandleFull();
        return;
    }

    status = LPUART_TransferGetReceiveCountEDMA(
                 LPUART1,
                 &g_lpuartEdmaHandle,
                 &count);

    /* 再次检查 DMA 完成标志 */
    if (s_rxDmaFullPending != 0U)
    {
        RxHandleFull();
        return;
    }

    if (status != kStatus_Success)
    {
        g_rxStateError++;

        /*
         * 没有正在进行的 DMA 传输时，尝试恢复接收。
         * 这属于异常恢复路径。
         */
        if (status == kStatus_NoTransferInProgress)
        {
            if (RearmRx() != kStatus_Success)
            {
                g_rxRearmError++;
            }
        }

        return;
    }

    /* 没有新数据，不必终止和重新启动 DMA */
    if (count == 0U)
    {
        LPUART_EnableRxDMA(LPUART1, true);
        return;
    }

    /* 收到部分数据 */
    if (count < NMEA_BUF_SIZE)
    {
        LPUART_TransferAbortReceiveEDMA(
            LPUART1, &g_lpuartEdmaHandle);

        RingWrite(s_rxDmaBuf, count);

        if (RearmRx() != kStatus_Success)
        {
            g_rxRearmError++;
        }

        return;
    }

    /*
     * count == NMEA_BUF_SIZE：
     * DMA 已经收满，等待 DMA 完成中断触发回调。
     * 不要在这里再次写入缓冲区或重启 DMA。
     */
}

static void init_pins_lpuart0()
{
    /* LPUART0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);

    const port_pin_config_t port0_2_pin78_config = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPUART0_RXD */
                                                    kPORT_MuxAlt2,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT0_2 (pin 78) is configured as LPUART0_RXD */
    PORT_SetPinConfig(PORT0, 2U, &port0_2_pin78_config);

    const port_pin_config_t port0_3_pin79_config = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPUART0_TXD */
                                                    kPORT_MuxAlt2,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT0_3 (pin 79) is configured as LPUART0_TXD */
    PORT_SetPinConfig(PORT0, 3U, &port0_3_pin79_config);
}

static void init_pins_lpuart1()
{
    /* LPUART1 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kLPUART1_RST_SHIFT_RSTn);
    /* Release peripheral reset */
    RESET_ReleasePeripheralReset(kDMA_RST_SHIFT_RSTn);

    const port_pin_config_t port3_20_pin55_config = {/* Internal pull-up resistor is enabled */
                                                     kPORT_PullUp,
                                                     /* Low internal pull resistor value is selected. */
                                                     kPORT_LowPullResistor,
                                                     /* Fast slew rate is configured */
                                                     kPORT_FastSlewRate,
                                                     /* Passive input filter is disabled */
                                                     kPORT_PassiveFilterDisable,
                                                     /* Open drain output is disabled */
                                                     kPORT_OpenDrainDisable,
                                                     /* Low drive strength is configured */
                                                     kPORT_LowDriveStrength,
                                                     /* Normal drive strength is configured */
                                                     kPORT_NormalDriveStrength,
                                                     /* Pin is configured as LPUART1_RXD */
                                                     kPORT_MuxAlt3,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT3_20 (pin 55) is configured as LPUART1_RXD */
    PORT_SetPinConfig(PORT3, 20U, &port3_20_pin55_config);

    const port_pin_config_t port3_21_pin54_config = {/* Internal pull-up resistor is enabled */
                                                     kPORT_PullUp,
                                                     /* Low internal pull resistor value is selected. */
                                                     kPORT_LowPullResistor,
                                                     /* Fast slew rate is configured */
                                                     kPORT_FastSlewRate,
                                                     /* Passive input filter is disabled */
                                                     kPORT_PassiveFilterDisable,
                                                     /* Open drain output is disabled */
                                                     kPORT_OpenDrainDisable,
                                                     /* Low drive strength is configured */
                                                     kPORT_LowDriveStrength,
                                                     /* Normal drive strength is configured */
                                                     kPORT_NormalDriveStrength,
                                                     /* Pin is configured as LPUART1_TXD */
                                                     kPORT_MuxAlt3,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT3_21 (pin 54) is configured as LPUART1_TXD */
    PORT_SetPinConfig(PORT3, 21U, &port3_21_pin54_config);
}

void init_lpuart0() /*本函数相当于SDK的InitDebugConsole函数*/
{
    init_pins_lpuart0();
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    RESET_PeripheralReset(BOARD_DEBUG_UART_RST);

    DbgConsole_Init(BOARD_DEBUG_UART_INSTANCE, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE,
                    BOARD_DEBUG_UART_CLK_FREQ);
}

static void LPUART1_UserCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData)
{
    if ((base == LPUART1) && (status == kStatus_LPUART_RxIdle))
    {
        /*
         * DMA 已经收满 NMEA_BUF_SIZE。
         * 这里只通知 UART 中断，不在这里操作接收缓冲区。
         */
        s_rxDmaFullPending = 1U;

        __DMB();

        /* 软件触发 LPUART1 中断 */
        NVIC_SetPendingIRQ(LPUART1_IRQn);
    }
}

static void RxHandleFull(void)
{
    if (s_rxDmaFullPending != 0U)
    {
        s_rxDmaFullPending = 0U;

        /* 整块 DMA 缓冲区已经接收完成 */
        RingWrite(s_rxDmaBuf, NMEA_BUF_SIZE);

        /* 重新启动 DMA */
        if (RearmRx() != kStatus_Success)
        {
            g_rxRearmError++;
        }
    }
}

static status_t RearmRx(void)
{
    lpuart_transfer_t xfer;

    xfer.data = s_rxDmaBuf;
    xfer.dataSize = NMEA_BUF_SIZE;

    return LPUART_ReceiveEDMA(LPUART1, &g_lpuartEdmaHandle, &xfer);
}

void init_lpuart1()
{
    init_pins_lpuart1();
    CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPUART1);

    lpuart_config_t config;
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    LPUART_Init(LPUART1, &config, BOARD_DEBUG_UART_CLK_FREQ);

#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
    /* Init DMAMUX */
    DMAMUX_Init(DMA0);
    /* Set channel for LPUART */
    DMAMUX_SetSource(DMA0, LPUART_TX_DMA_CHANNEL, LPUART_TX_DMA_REQUEST);
    DMAMUX_SetSource(DMA0, LPUART_RX_DMA_CHANNEL, LPUART_RX_DMA_REQUEST);
    DMAMUX_EnableChannel(DMA0, LPUART_TX_DMA_CHANNEL);
    DMAMUX_EnableChannel(DMA0, LPUART_RX_DMA_CHANNEL);
#endif
    /* Init the EDMA module */
    edma_config_t userConfig = {0};
    EDMA_GetDefaultConfig(&userConfig);
    EDMA_Init(DMA0, &userConfig);

    /* 设置 DMA 和 LPUART 中断优先级 */
    NVIC_SetPriority(DMA_CH0_IRQn, RX_IRQ_PRIORITY);
    NVIC_SetPriority(DMA_CH1_IRQn, RX_IRQ_PRIORITY);
    NVIC_SetPriority(LPUART1_IRQn, RX_IRQ_PRIORITY);

    EDMA_CreateHandle(&g_lpuartTxEdmaHandle, DMA0, LPUART_TX_DMA_CHANNEL);
    EDMA_CreateHandle(&g_lpuartRxEdmaHandle, DMA0, LPUART_RX_DMA_CHANNEL);
#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
    EDMA_SetChannelMux(DMA0, LPUART_TX_DMA_CHANNEL, LPUART1_TX_EDMA_CHANNEL);
    EDMA_SetChannelMux(DMA0, LPUART_RX_DMA_CHANNEL, LPUART1_RX_EDMA_CHANNEL);
#endif
    /* Create LPUART DMA handle. */
    LPUART_TransferCreateHandleEDMA(LPUART1, &g_lpuartEdmaHandle, LPUART1_UserCallback, NULL, &g_lpuartTxEdmaHandle,
                                    &g_lpuartRxEdmaHandle);

    if (RearmRx() != kStatus_Success)
    {
        g_rxRearmError++;
    }
    /* 第 3 层：打开 IDLE 检测（ILT=1，空闲计数从停止位后开始，判定更准） */
    LPUART1->CTRL |= LPUART_CTRL_ILIE_MASK | LPUART_CTRL_ILT_MASK;

    /* 第 2 层：NVIC 解除屏蔽 —— 关联链的最后一环 */
    EnableIRQ(LPUART1_IRQn);
}

/*══════════ 环形缓冲（单生产者=中断侧，单消费者=主循环）══════════*/
#define RING_SIZE   4096U                     /* 2 的幂 */
#define RING_MASK   (RING_SIZE - 1U)

static uint8_t           s_ring[RING_SIZE];
static volatile uint32_t s_wr = 0U;           /* 自由计数值，只在 ISR 侧推进 */
static volatile uint32_t s_rd = 0U;           /* 只在主循环侧推进 */
volatile uint32_t        g_ringOvf = 0U;      /* 溢出计数：新的健康指标 */

/*--------- ISR 侧调用：写入一批 ---------*/
static void RingWrite(const uint8_t *p, uint32_t n)
{
    uint32_t free_ = RING_SIZE - (s_wr - s_rd);

    if (n > free_) { g_ringOvf++; n = free_; }        /* 放不下就丢新，计数 */

    uint32_t w    = s_wr & RING_MASK;
    uint32_t head = RING_SIZE - w;
    if (head > n) head = n;
    memcpy(&s_ring[w], p, head);                        /* 尾段 */
    memcpy(s_ring, p + head, n - head);                 /* 回卷段 */
    s_wr += n;                          /* 一次性推进：读者永远看不到半写状态 */
}

/*--------- 主循环侧调用：尽力读出 ---------*/
uint32_t NMEA_RxRead(uint8_t *dst, uint32_t max)
{
    uint32_t avail = s_wr - s_rd;
    if (avail > max) avail = max;

    uint32_t r    = s_rd & RING_MASK;
    uint32_t head = RING_SIZE - r;
    if (head > avail) head = avail;
    memcpy(dst, &s_ring[r], head);
    memcpy(dst + head, s_ring, avail - head);
    s_rd += avail;
    return avail;
}

/*═══════════════════════════════════════════════════════════
 *  中断服务函数 —— 第 1 层的"强定义"
 *  名字必须与 startup_MCXA156.c 向量表中的弱符号完全一致，
 *  链接时自动完成覆盖。用 LPUART 其它实例则名字跟着改
 *═══════════════════════════════════════════════════════════*/
 void LPUART1_IRQHandler(void)
{
    uint32_t stat = LPUART1->STAT;

    bool idle = ((stat & LPUART_STAT_IDLE_MASK) != 0U);

    if (idle)
    {
        /* W1C：直接写 1 清除 IDLE */
        LPUART1->STAT = LPUART_STAT_IDLE_MASK;
    }

    /*
     * 先处理 DMA 满缓冲区事件。
     */
    if (s_rxDmaFullPending != 0U)
    {
        RxHandleFull();
    }

    /*
     * 再处理串口空闲事件。
     */
    if (idle)
    {
        RxHandleIdle();
    }

    SDK_ISR_EXIT_BARRIER;   /* 与 SDK 自带 handler 保持一致，fsl_common.h 提供 */
}
