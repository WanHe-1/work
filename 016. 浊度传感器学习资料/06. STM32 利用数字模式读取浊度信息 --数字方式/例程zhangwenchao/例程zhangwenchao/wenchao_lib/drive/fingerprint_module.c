#include "fingerprint_module.h"

u8 RI = 0;
u8 SBUF = 0;

unsigned char confir_code=11;
unsigned char sum[2];
int summaf,summas;

GPIOZWC_FINGER gpiozwc_finger = { HW_GPIOA,  GPIO_Pin_8, HW_UART3, 9600};

void uart_finger_int(uint16_t Res){	
	
	RI = 1; SBUF = Res;
  UART_WriteByte(HW_UART1, Res);
}

void fingerprint_init(){ 
	GPIO_QuickInit(gpiozwc_finger.HW_GPIOx, gpiozwc_finger.GPIO_Pin_x , GPIO_Mode_IPU);//初始化与LED连接的硬件接口
	UART_QuickInit(gpiozwc_finger.HW_UARTx, gpiozwc_finger.bound, 2, 2,  ENABLE);
	UART_CallbackInstall(gpiozwc_finger.HW_UARTx, uart_finger_int);
}

uint8_t is_wak_finger(){
	return GPIO_ReadBit( gpiozwc_finger.HW_GPIOx, gpiozwc_finger.GPIO_Pin_x );
}

unsigned char img_ins[12] = {0xef ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF ,0x01 ,0x00 ,0x03 ,0x01 ,0x00 ,0x05};

u8 SFG_getimage(){			     

  unsigned char i;
	for(i=0; i<12; i++){
		UART_WriteByte(gpiozwc_finger.HW_UARTx , img_ins[i]);
		SYSTICK_DelayMs(5);
	}
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
        
   while(RI==0){} RI=0; confir_code=SBUF;
   while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]<<8)+sum[0];		

	 return confir_code;
}



unsigned char genchar_ins[13] = {0xef ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF ,0x01 ,0x00 ,0x04 ,0x02 ,0x01 ,0x00 ,0x08};

u8 SFG_genchar(){
  unsigned char i;
  for(i=0; i<13; i++){
		UART_WriteByte(gpiozwc_finger.HW_UARTx , genchar_ins[i]);
		SYSTICK_DelayMs(5);
	}
        
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 
   while(RI==0){} RI=0; confir_code=SBUF;
   while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]<<8)+sum[0];		
	 return confir_code;		 
}

unsigned char fastsearch_ins[17] = {0xEF ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF ,0x01 ,0x00 ,0x08 ,0x1B ,0x01 ,0x00 ,0x00 ,0x00 ,0xB4 ,0x00 ,0xD9 };

u8 SFG_fastsearch() { //????????ID?   sum?pagenum>255??????@@@
  unsigned char i,ID1,ID2;
	for(i=0; i<17; i++){
		UART_WriteByte(gpiozwc_finger.HW_UARTx , fastsearch_ins[i]);
		SYSTICK_DelayMs(5);
	}
	
   for(i=0;i<9;i++){ while(RI==0){} RI=0;}
	        
   while(RI==0){} RI=0; confir_code=SBUF;
   while(RI==0){} RI=0; ID1=SBUF;
   while(RI==0){} RI=0; ID2=SBUF;				  //????ID?
   while(RI==0){} RI=0;
   while(RI==0){} RI=0;
   while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]<<8)+sum[0];									 
   PageID=(ID1<<8)+ID2;
	 return confir_code;
}

unsigned char enroll_ins[17] = {0xEF ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF , 0X01, 0X00, 0X03, 0X10, 0X00, 0x14};

u8 SFG_enroll(){				
   unsigned char i,ID1,ID2;
   
	for(i=0; i<12; i++){
		UART_WriteByte(gpiozwc_finger.HW_UARTx , enroll_ins[i]);
		SYSTICK_DelayMs(5);
	}
   for(i=0;i<9;i++){  while(RI==0){} RI=0;}
   while(RI==0){} RI=0; confir_code=SBUF;
   while(RI==0){} RI=0; ID1=SBUF;
   while(RI==0){} RI=0; ID2=SBUF;
   while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]<<8)+sum[0];									 
   PageID=(ID1<<8)+ID2;
	 return confir_code;
}

unsigned char deletchar_ins[10] = {0xEF ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF , 0X01, 0X00, 0X07, 0X0c};

u8 SFG_deletchar(unsigned int pageID){   //????		     ????2??????????????
  unsigned char i,ID1,ID2;

	for(i=0; i<10; i++){
	  UART_WriteByte(gpiozwc_finger.HW_UARTx , deletchar_ins[i]);
		SYSTICK_DelayMs(5);
	}				

   ID1=pageID;ID2=pageID>>8;
   SBUF=ID2;
	
		UART_WriteByte(gpiozwc_finger.HW_UARTx , ID2);
		SYSTICK_DelayMs(5);
		UART_WriteByte(gpiozwc_finger.HW_UARTx , ID1);
		SYSTICK_DelayMs(5);
		UART_WriteByte(gpiozwc_finger.HW_UARTx , 0X00);
		SYSTICK_DelayMs(5);
		UART_WriteByte(gpiozwc_finger.HW_UARTx , 1);
		SYSTICK_DelayMs(5);
        
   summaf=0x15+ID1+ID2;
   sum[0]=summaf;
   sum[1]=summaf>>8;
   SBUF=sum[1];
	
	 UART_WriteByte(gpiozwc_finger.HW_UARTx , sum[1]);
	 SYSTICK_DelayMs(5);
	 UART_WriteByte(gpiozwc_finger.HW_UARTx , sum[0]);
	 SYSTICK_DelayMs(5);
        
   for(i=0;i<9;i++) { while(RI==0){} RI=0;}
   while(RI==0){} RI=0; confir_code=SBUF;
   while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]<<8)+sum[0];	
	 return confir_code;
}

unsigned char identify_ins[17] = {0xEF ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF , 0X01, 0X00, 0X03, 0X11, 0X00, 0x15};

u8 SFG_identify()	{
  unsigned char i,ID1,ID2; 
	
	for(i=0; i<12; i++){		
	 UART_WriteByte(gpiozwc_finger.HW_UARTx , identify_ins[i]);
	 SYSTICK_DelayMs(5);
	}

   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
   while(RI==0){} RI=0; confir_code=SBUF;
   while(RI==0){} RI=0; ID1=SBUF; 
   while(RI==0){} RI=0; ID2=SBUF;
   while(RI==0){} RI=0;
   while(RI==0){} RI=0;			  //??
   while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]>>8)+sum[0];									 
   PageID=(ID1<<8)+ID2;
	 return confir_code;
}

unsigned char empty_ins[17] = {0xEF ,0x01 ,0xFF ,0xFF ,0xFF ,0xFF , 0X01, 0X00, 0X03, 0X0d, 0X00, 0x11};

u8 SFG_empty(){
  unsigned char i; 
	
	for(i=0; i<12; i++){
	 UART_WriteByte(gpiozwc_finger.HW_UARTx , empty_ins[i]);
	 SYSTICK_DelayMs(5);
	}
	
	 while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0;
   while(RI==0){} RI=0;
	 while(RI==0){} RI=0; confir_code=SBUF;
	 while(RI==0){} RI=0; sum[1]=SBUF;
   while(RI==0){} RI=0; sum[0]=SBUF;
   summas=(sum[1]>>8)+sum[0];	
	 return confir_code;
}

