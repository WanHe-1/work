#include "lcd1602.h"

/**
 * @param[in]   void
 * @note        GPIO初始化 
 */
void GPIO_Configuration(void)
{
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_RS, gpiozwc_1602.GPIO_Pin_x_RS, GPIO_Mode_Out_PP); 
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_RW, gpiozwc_1602.GPIO_Pin_x_RW, GPIO_Mode_Out_PP); 
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_EN, gpiozwc_1602.GPIO_Pin_x_EN, GPIO_Mode_Out_PP); 
	
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_DB4, gpiozwc_1602.GPIO_Pin_x_DB4, GPIO_Mode_Out_PP); 
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_DB5, gpiozwc_1602.GPIO_Pin_x_DB5, GPIO_Mode_Out_PP); 
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_DB6, gpiozwc_1602.GPIO_Pin_x_DB6, GPIO_Mode_Out_PP); 
	GPIO_QuickInit( gpiozwc_1602.HW_GPIOx_DB7, gpiozwc_1602.GPIO_Pin_x_DB7, GPIO_Mode_Out_PP); 
}
 
/**
 * @param[in]   void
 * @note        检测液晶处于忙状态，等待液晶不忙
 */
void LCD1602_Wait_Ready(void)
{
	int8_t sta;
	
	DATAOUT(0xff);    //端口置高
	LCD_RS_Clr();     //RS 0
	LCD_RW_Set();     //RW 1
	do
	{
		LCD_EN_Set();   //EN 1
		SYSTICK_DelayMs(5);	 
 		sta =  GPIO_ReadBit(gpiozwc_1602.HW_GPIOx_DB7, gpiozwc_1602.GPIO_Pin_x_DB7);//读取状态字
		LCD_EN_Clr();  //EN  0
	}while(sta & 0x80);
}
 
/**
 * @param[in]   cmd            液晶命令指令
 * @note        给1602写指令
 */
void LCD1602_Write_Cmd(u8 cmd)
{
	LCD1602_Wait_Ready();   
	LCD_RS_Clr();   
	LCD_RW_Clr();
	DATAOUT(cmd);   
	LCD_EN_Set();
	LCD_EN_Clr();
	
	DATAOUT(cmd<<4);  
	LCD_EN_Set();
	LCD_EN_Clr();
}
 
/**
 * @param[in]   dat     数据
 * @note        给1602写数据
 */
void LCD1602_Write_Dat(u8 dat)
{
	LCD1602_Wait_Ready();   
	LCD_RS_Set();   //1
	LCD_RW_Clr();   //0
	
	DATAOUT(dat);    
	LCD_EN_Set();  
	LCD_EN_Clr();
	
	DATAOUT(dat<<4);  
	LCD_EN_Set(); 
	LCD_EN_Clr();
	
}
 
/**
 * @param[in]   void
 * @note        清屏操作
 */
void LCD1602_ClearScreen(void)
{
	LCD1602_Write_Cmd(0x01);
	
}
 
/**
 * @param[in]   x   定位x位置
 * @param[in]   y   定位y位置
 * @note        给1602写指令
 */
void LCD1602_Set_Cursor(u8 x, u8 y)
{
	u8 addr;
	
	if (y == 0)
		addr = 0x00 + x;
	else
		addr = 0x40 + x;
	LCD1602_Write_Cmd(addr | 0x80);
}
 
/**
 * @param[in]   x   定位x位置
 * @param[in]   y   定位y位置
 * @note        给1602写指令
 */
void LCD1602_Show_Str(u8 x, u8 y, u8 *str)
{
	LCD1602_Set_Cursor(x, y);
	while(*str != '\0')
	{
		LCD1602_Write_Dat(*str++);
	}
}
 
/**
 * @param[in]   void
 * @note        1602初始化
 */
void LCD1602_Init(void)
{
	GPIO_Configuration();
	LCD1602_Write_Cmd(0x28);	//四位数据模式
	LCD1602_Write_Cmd(0x0C);	 
	LCD1602_Write_Cmd(0x06);	 
	LCD1602_Write_Cmd(0x01); 
}


