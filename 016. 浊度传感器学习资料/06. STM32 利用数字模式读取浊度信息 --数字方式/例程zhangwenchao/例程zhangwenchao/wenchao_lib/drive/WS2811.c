
#include "WS2811.h"
	
unsigned long WsDat[nWs];   //显存
unsigned long WsDat_RUN[nWs];   //流水灯显存


/**************************************************
* IO初始化（移植时请修改）
***************************************************/
void WS_GPIO_Init(int instance, int GPIO_Pin_x)
{
	GPIO_QuickInit(instance, GPIO_Pin_x, GPIO_Mode_Out_PP);
}

/**************************
* 内部延时
***************************/

void WS_delay_us(__IO u32 nCount)	 //简单的延时函数
{
	for(; nCount != 0; nCount--);
} 

void WS_delay2us()
{
	unsigned char i;
	for(i=0; i<11; i++);
}

void WS_delay05us()
{
//	unsigned char i;
//	for(i=0; i<1; i++);
}

void WS_delay_ms(uint32_t nms)
{	
	while(nms--)
	{
	  WS_delay_us(1670);	//普通方式延时 		
	}
}


/***************************
* 发送一比特
****************************/
void TX0(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin)    	// 发送0	
{
	GPIO_SetBits(GPIOx, GPIO_Pin); 
    WS_delay05us();
	GPIO_ResetBits(GPIOx, GPIO_Pin);
	WS_delay2us();
} 
void TX1(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin) 	  // 发送1	
{ 							 	
	GPIO_SetBits(GPIOx, GPIO_Pin); 
	WS_delay2us();
	GPIO_ResetBits(GPIOx, GPIO_Pin);
	WS_delay05us();
} 
void WS_Reset(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin)   //复位
{
	GPIO_ResetBits(GPIOx, GPIO_Pin); 
	WS_delay_us(50); 
	GPIO_SetBits(GPIOx, GPIO_Pin);
	GPIO_ResetBits(GPIOx, GPIO_Pin); 
}

/************************************************
* 发送数据
*************************************************/
void WS_Set1(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,unsigned long dat)
{
	unsigned char i;	
	for(i=0; i<24; i++)
	{
		if(0x800000 == (dat & 0x800000) )	
			TX1(GPIOx,GPIO_Pin);
		else								
			TX0(GPIOx,GPIO_Pin);
		dat<<=1;        //左移一位
	}
}


void WS_Set_Date(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,unsigned char len,unsigned long dat) // 全部发送同样的数据
{
   unsigned char i;
   for(i = 0; i <len; i++)
   {
		WS_Set1(GPIOx,GPIO_Pin,dat);	
   }
	WS_Reset(GPIOx,GPIO_Pin);
}


void WS_SetAll(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin)   //发送所有字节
{
	unsigned char i;
	
	for(i=0;i<nWs;i++)
	{
		WS_Set1(GPIOx,GPIO_Pin,WsDat_RUN[i]);  
	}
	WS_Reset(GPIOx,GPIO_Pin);
	WS_delay_ms(500);	
}



/**************************颜色渐变函数***************************/
/*
从起始颜色---最终颜色
*/
unsigned char abs0(int num)//求绝对值
{
	if(num>0) return num;
	
	num = -num;
	return (unsigned char) num;
}

unsigned long WS_SET_ColorToColor(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,unsigned long color0, unsigned long color1,unsigned int speed)  
{
	unsigned char Red0, Green0, Blue0;  // 起始三原色
	unsigned char Red1, Green1, Blue1;  // 结果三原色
	int			  RedMinus, GreenMinus, BlueMinus;	// 颜色差（color1 - color0）
	unsigned char NStep; 							// 需要几步
	float		  RedStep, GreenStep, BlueStep;		// 各色步进值
	unsigned long color;							// 结果色
	unsigned char i;
	
	// 绿 红 蓝 三原色分解
	Red0   = color0>>8;
	Green0 = color0>>16;
	Blue0  = color0;
	
	Red1   = color1>>8;
	Green1 = color1>>16;
	Blue1  = color1;
	
	// 计算需要多少步（取差值的最大值）
	RedMinus   = Red1 - Red0; 
	GreenMinus = Green1 - Green0; 
	BlueMinus  = Blue1 - Blue0;
	
	//判断两个绝对值大小。。a>b为真，则=a；否则=b
	NStep = ( abs0(RedMinus) > abs0(GreenMinus) ) ? abs0(RedMinus):abs0(GreenMinus); 
	NStep = ( NStep > abs0(BlueMinus) ) ? NStep:abs0(BlueMinus);
	
	// 计算出各色步进值
	RedStep   = (float)RedMinus   / NStep;
	GreenStep = (float)GreenMinus / NStep;
	BlueStep  = (float)BlueMinus  / NStep;
	
	// 渐变开始
	for(i=0; i<NStep; i++)
	{
		Red1   = Red0   + (int)(RedStep   * i);
		Green1 = Green0 + (int)(GreenStep * i);
		Blue1  = Blue0  + (int)(BlueStep  * i);
		
		color  = Green1<<16 | Red1<<8 | Blue1; 	// 合成  绿红蓝
		
		WS_Set_Date(GPIOx,GPIO_Pin,nWs,color); //发送颜色，所有灯带同色
		WS_delay_ms(speed);
	}
	return color;
}


/*************跑马灯****************
000000000000000000000000
111100000000000000000000
000011110000000000000000
000000001111000000000000
111100000000111100000000
000011110000000011110000
***********************************/
void WS_Set_Color(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,unsigned long color) //初始化显存数组
{
	unsigned char i;
	for(i=0;i<50;i++)
		WsDat_RUN[i]=color;
	WS_SetAll(GPIOx,GPIO_Pin);
}

//入口参数     color----speed----temp
//            显示颜色--流水速度--间隔距离
void RUN_LED(GPIO_TypeDef *GPIOx,uint16_t GPIO_Pin,unsigned long color,unsigned int speed,unsigned char temp)//跑马灯
{
	unsigned char i,j;
	for(j=0;j<(nWs-1);j++)
	{
		for(i=nWs;i>0;i--)
			WsDat_RUN[i] = WsDat_RUN[i-1];
		
		if(j%temp == 0)
			WsDat_RUN[0] = color;		
		else
			WsDat_RUN[0] = Black;
		
		//WS_Set_Date(GPIO_PIN,nWs,WsDat_RUN[i]); 
		WS_SetAll(GPIOx,GPIO_Pin);
		WS_delay_ms(speed);
	}
}



