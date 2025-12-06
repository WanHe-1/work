#include "GSM800C.h"

u8 gsm_buf[GSM_BUF_MAX];

u8 Times=0,First_Int = 0,shijian=0;

u8 Timer0_start = 0;	 

u8 Find( u8 *a)
{ 
  if(strstr(gsm_buf,a)!=NULL)
	    return 1;
	else
			return 0;
}


void CLR_Gsm_Buf(void)
{
	u16 k;
	for(k=0;k<GSM_BUF_MAX;k++)      
	{
		gsm_buf[k] = 0x00;
	}
    First_Int = 0;             
}


void Second_Gsm_AT_Command(u8 *b,u8 *a,u8 wait_time)         
{
	u8 i;
	u8 *c;
	c = b;									 
	CLR_Gsm_Buf(); 
  i = 0;
	while(i == 0)                    
	{
		if(!Find(a))           
		{
			if(Timer0_start == 0)
			{
				b = c;						
				for ( ; *b!='\0';b++)
				{
					UART_WriteByte(GSM_UART, *b);
					SYSTICK_DelayMs(5);
				}
				UART_SendLR(GSM_UART);	
				Times = 0;
				shijian = wait_time;
				Timer0_start = 1; 
		   }
    }
 	  else
		{
			i = 1;
			Timer0_start = 0;  
		}
	}
	CLR_Gsm_Buf(); 
}


void GSM_timer()
{	
	static unsigned int time = 0;
	if (TIM_GetITStatus(GSM_TIMX, TIM_IT_Update) != RESET){  //检查TIM3更新中断发生与否
				
			if(time++ > 50000){ time = 0; }
		
			if(Timer0_start){
				Times++;
				if(Times > 230) Times = 200;
			}
			
			if(Times > (50*shijian)){
				Timer0_start = 0;
				Times = 0;
			}

			TIM_ClearITPendingBit(GSM_TIMX, TIM_IT_Update);  //清除TIMx更新中断标志 
		}
}


void GSM_uart(uint16_t Res)
{		
  	UART_WriteByte(HW_UART1, Res);
		gsm_buf[First_Int] = Res;  	   
		First_Int++;                		 
		if(First_Int > GSM_BUF_MAX)       		 
		{
			First_Int = 0;
		}
}

void GSM800C_init(void){
	UART_QuickInit(GSM_UART, 9600, 2, 2, ENABLE);
	UART_CallbackInstall(GSM_UART, GSM_uart);
	
	TIM_us_Init(TIM2, 20000, TIM_IT_Update,2, ENABLE);	
 	TIMER_CallbackInstall(HW_TIMER2, GSM_timer);

}

void Set_EN_Text_Mode(void)
{
	Second_Gsm_AT_Command("AT","OK",3);	
	Second_Gsm_AT_Command("AT&F","OK",3);	
	Second_Gsm_AT_Command("AT+CSCS=\"GSM\"","OK",3);
	Second_Gsm_AT_Command("AT+CMGF=1","OK",3);	
}

void Set_CH_Text_Mode(void)
{
	Second_Gsm_AT_Command("AT","OK",3);	
	Second_Gsm_AT_Command("AT&F","OK",3);	
	Second_Gsm_AT_Command("AT+CMGF=1","OK",3);	
	Second_Gsm_AT_Command("AT+CSMP=17,167,0,8","OK",3);
	//Second_Gsm_AT_Command("AT+CSMP=17,167,2,25","OK",3);
	Second_Gsm_AT_Command("AT+CSCS=\"UCS2\"","OK",3);
}



void Second_Gsm_SMS_Command(u8 *a,u8 wait_time)         
{
	u8 i;							 
	CLR_Gsm_Buf(); 
  i = 0;
	while(i == 0)                    
	{
		if(!Find(a))           
		{
			if(Timer0_start == 0)
			{
				UART_WriteByte(GSM_UART, 0X1A);
				SYSTICK_DelayMs(5);
				Times = 0;
				shijian = wait_time;
				Timer0_start = 1; 
		   }
    }
 	  else
		{
			i = 1;
			Timer0_start = 0;  
		}
	}
	CLR_Gsm_Buf(); 
}

void Send_Text_Sms(u8 *tel,u8 *txt)
{
	Second_Gsm_AT_Command(tel,">",3); //??????
	UART_SendString(GSM_UART, txt);     //??????
	SYSTICK_DelayMs(200);
	//Second_Gsm_SMS_Command("+", 60);
	UART_WriteByte(GSM_UART, 0X1A);
	SYSTICK_DelayMs(15000);
}


