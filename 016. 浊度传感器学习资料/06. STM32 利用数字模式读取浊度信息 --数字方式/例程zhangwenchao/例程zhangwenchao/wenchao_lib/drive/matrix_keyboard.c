/**
  ******************************************************************************
  * @file    matric_keyboard.c
  * @author  张文超 
	* @qq      269426626
  * @version V1.0
  * @date    2016.8.22
  * @note    此程序为ARM中uart的底层编程（依附于官方底层库的二次封装）
  ******************************************************************************
  */

#include "matrix_keyboard.h"


//修改这八个序号，就可以修改矩阵键盘的键值了
GPIOZWC_MAXTRIC gpiozwc_maxtric[8] = {
	{HW_GPIOA, GPIO_Pin_15},
	{HW_GPIOB, GPIO_Pin_3},
	{HW_GPIOB, GPIO_Pin_4},
	{HW_GPIOB, GPIO_Pin_5},
	{HW_GPIOB, GPIO_Pin_6},
	{HW_GPIOB, GPIO_Pin_7},
	{HW_GPIOB, GPIO_Pin_8},
	{HW_GPIOB, GPIO_Pin_9}
};


u8 get_maxtric_keybord_value(){
	u8 cord_h=0XFF,cord_l=0XFF;
	u8 key_v = 255;
	
	for(int i = 0; i < 4; i++ ){
		GPIO_QuickInit( gpiozwc_maxtric[i].HW_GPIOx, gpiozwc_maxtric[i].GPIO_Pin_x, GPIO_Mode_Out_PP );
		GPIO_Writebit( gpiozwc_maxtric[i].HW_GPIOx, gpiozwc_maxtric[i].GPIO_Pin_x, 0 );
		GPIO_QuickInit( gpiozwc_maxtric[i+4].HW_GPIOx, gpiozwc_maxtric[i+4].GPIO_Pin_x, GPIO_Mode_IPU );
	}
		
	cord_l&=(u8)((GPIO_ReadBit(gpiozwc_maxtric[4].HW_GPIOx, gpiozwc_maxtric[4].GPIO_Pin_x) << 0)|(GPIO_ReadBit(gpiozwc_maxtric[5].HW_GPIOx, gpiozwc_maxtric[5].GPIO_Pin_x) << 1)|

               (GPIO_ReadBit(gpiozwc_maxtric[6].HW_GPIOx, gpiozwc_maxtric[6].GPIO_Pin_x) << 2)|(GPIO_ReadBit(gpiozwc_maxtric[7].HW_GPIOx, gpiozwc_maxtric[7].GPIO_Pin_x) << 3));
	
	
	if(cord_l!=0X0F){
			SYSTICK_DelayMs(10);	
		
		cord_l&=(u8)((GPIO_ReadBit(gpiozwc_maxtric[4].HW_GPIOx, gpiozwc_maxtric[4].GPIO_Pin_x) << 0)| (GPIO_ReadBit(gpiozwc_maxtric[5].HW_GPIOx, gpiozwc_maxtric[5].GPIO_Pin_x) << 1)|

								 (GPIO_ReadBit(gpiozwc_maxtric[6].HW_GPIOx, gpiozwc_maxtric[6].GPIO_Pin_x) << 2)| (GPIO_ReadBit(gpiozwc_maxtric[7].HW_GPIOx, gpiozwc_maxtric[7].GPIO_Pin_x) << 3));
	
		if(cord_l!=0X0F){
			
				for(int i = 0; i < 4; i++ ){
					GPIO_QuickInit( gpiozwc_maxtric[i+4].HW_GPIOx, gpiozwc_maxtric[i+4].GPIO_Pin_x, GPIO_Mode_Out_PP );
					GPIO_Writebit( gpiozwc_maxtric[i+4].HW_GPIOx, gpiozwc_maxtric[i+4].GPIO_Pin_x, 0 );
					GPIO_QuickInit( gpiozwc_maxtric[i].HW_GPIOx, gpiozwc_maxtric[i].GPIO_Pin_x, GPIO_Mode_IPU );
				}

			cord_h&=(u8)((GPIO_ReadBit(gpiozwc_maxtric[0].HW_GPIOx, gpiozwc_maxtric[0].GPIO_Pin_x) << 0)| (GPIO_ReadBit(gpiozwc_maxtric[1].HW_GPIOx, gpiozwc_maxtric[1].GPIO_Pin_x) << 1)|

								 (GPIO_ReadBit(gpiozwc_maxtric[2].HW_GPIOx, gpiozwc_maxtric[2].GPIO_Pin_x) << 2)| (GPIO_ReadBit(gpiozwc_maxtric[3].HW_GPIOx, gpiozwc_maxtric[3].GPIO_Pin_x) << 3));

			switch(cord_h << 4 | cord_l){ 
				case 0xe7: key_v = 10; break;  
				case 0xd7: key_v = 11; break; 
				case 0xb7: key_v = 12; break; 
				case 0x77: key_v = 13; break; 
				case 0xeb: key_v = 3; break; 
				case 0xdb: key_v = 6; break; 
				case 0xbb: key_v = 9; break; 
				case 0x7b: key_v = 15; break; 
				case 0xed: key_v = 2; break; 
				case 0xdd: key_v = 5; break; 
				case 0xbd: key_v = 8;break; 
				case 0x7d: key_v = 0;break; 
				case 0xee: key_v = 1;break; 
				case 0xde: key_v = 4;break; 
				case 0xbe: key_v = 7;break; 
				case 0x7e: key_v = 14;break; 
			}
			
			while(cord_h != 0x0f){
				cord_h = 0x0f;
				cord_h&=(u8)((GPIO_ReadBit(gpiozwc_maxtric[0].HW_GPIOx, gpiozwc_maxtric[0].GPIO_Pin_x) << 0)| (GPIO_ReadBit(gpiozwc_maxtric[1].HW_GPIOx, gpiozwc_maxtric[1].GPIO_Pin_x) << 1)|
									 (GPIO_ReadBit(gpiozwc_maxtric[2].HW_GPIOx, gpiozwc_maxtric[2].GPIO_Pin_x) << 2)| (GPIO_ReadBit(gpiozwc_maxtric[3].HW_GPIOx, gpiozwc_maxtric[3].GPIO_Pin_x) << 3));
			}
		}		
	}
	
	return key_v;
	
}

