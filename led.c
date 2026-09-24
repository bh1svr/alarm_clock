#include "board.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include <stdint.h>

#define uchar uint8_t
#define uint  uint32_t

#define GPCS   GPIO3
#define PINCS  1U
#define GPCLK  GPIO3
#define PINCLK 31U
#define GPDIN  GPIO1
#define PINDIN 14U
#define CS(x) GPIO_PinWrite(GPCS, PINCS, x);
#define CLK(x) GPIO_PinWrite(GPCLK, PINCLK, x);
#define DIN(x) GPIO_PinWrite(GPDIN, PINDIN, x);

static void init_pins_led()
{
    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    /* GPIO3: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    /* PORT0: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GatePORT1);
    /* PORT3: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GatePORT3);

    RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
    /* GPIO3 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);
    /* PORT0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
    /* PORT3 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);

    gpio_pin_config_t gpio_led_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };
    /* Initialize GPIO functionality on pin PIO3_12 (pin 63)  */
    GPIO_PinInit(GPIO3,  1U, &gpio_led_config);
    GPIO_PinInit(GPIO3, 31U, &gpio_led_config);
    GPIO_PinInit(GPIO1, 14U, &gpio_led_config);

    const port_pin_config_t port_led_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
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
                                                     kPORT_MuxAlt0,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    PORT_SetPinConfig(PORT3,  1U, &port_led_config);
    PORT_SetPinConfig(PORT3, 31U, &port_led_config);
    PORT_SetPinConfig(PORT1, 14U, &port_led_config);
}


void Delay_xms(uint x)
{
    SDK_DelayAtLeastUs(x * 1000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY); 
}

//--------------------------------------------

//功能：向MAX7219(U3)写入字节

//入口参数：DATA 

//出口参数：无

//说明：

void Write_Max7219_byte(uchar DATA)         
{
    	uchar i;    
        CS(0);

	    for(i=8;i>=1;i--)
        {		  
            CLK(0); 
            if (DATA&0x80)
            {
                DIN(1);
            }
            else
            {
                DIN(0);
            }
            DATA=DATA<<1;
            CLK(1);
        }                                 
}

//-------------------------------------------

//功能：向MAX7219写入数据

//入口参数：address、dat

//出口参数：无

//说明：

void Write_Max7219(uchar address,uchar dat)
{ 
     CS(0);
	 Write_Max7219_byte(address);           //写入地址，即数码管编号

     Write_Max7219_byte(dat);               //写入数据，即数码管显示数字 
     CS(1);
}

void Init_MAX7219(void)
{

 Write_Max7219(0x09, 0xff);       //译码方式：BCD码

 Write_Max7219(0x0a, 0x03);       //亮度

 Write_Max7219(0x0b, 0x07);       //扫描界限；4个数码管显示

 Write_Max7219(0x0c, 0x01);       //掉电模式：0，普通模式：1

 Write_Max7219(0x0f, 0x01);       //显示测试：1；测试结束，正常显示：0

}

void led_test(void)
{
 Delay_xms(50);
 init_pins_led();
 Init_MAX7219();

 Delay_xms(2000);

 Write_Max7219(0x0f, 0x00);       //显示测试：1；测试结束，正常显示：0

 Write_Max7219(1,8);

 Write_Max7219(2,7);

 Write_Max7219(3,6);

 Write_Max7219(4,5); 

 Write_Max7219(5,4);

 Write_Max7219(6,3);

 Write_Max7219(7,2);

 Write_Max7219(8,1);
}


