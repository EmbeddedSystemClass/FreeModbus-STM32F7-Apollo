#include "sys.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "mb.h"

int main(void)
{
    Cache_Enable();
    HAL_Init();
    Stm32_Clock_Init(432,25,2,9);
    delay_init(216);
	
	LED_Init();
	HAL_Delay(200);
	LED0(0);
	LED1(0);
	
	eMBInit(MB_RTU, 0x01, 0x01, 9600, MB_PAR_NONE);
	eMBEnable();
	
    while(1)
    {
		eMBPoll();
	}
}
