#include "sysinc.h"

u8 txbuf[33]={1,7,3,4};
u8 rxbuf[33]={1,9,3,4};

 int main(void)
 {	
	NVIC_init();
	SYSTICK_DelayInit();	    	 //延时函数初始化	  	  	
	GPIO_QuickInit(HW_GPIOC, GPIO_Pin_13, GPIO_Mode_Out_PP);//初始化与LED连接的硬件接口
	GPIO_QuickInit(HW_GPIOB, GPIO_Pin_9, GPIO_Mode_IPU);//初始化与LED连接的硬件接口
	UART_QuickInit(HW_UART1, 9600, 2, 2, ENABLE);

	while(1)
	{		
		PCout(13)=0;
		SYSTICK_DelayMs(900);	 //延时300ms
		PCout(13)=1;
		SYSTICK_DelayMs(900);	//延时300ms
		
		int value = PBin(9);
		printf("value is %d \n", value );
	}
 }

