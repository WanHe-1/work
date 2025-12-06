#include "DS1302.h"


GPIOZWC_DS1302 gpiozwc_ds1302 = {HW_GPIOA, GPIO_Pin_4, HW_GPIOA, GPIO_Pin_5, HW_GPIOA, GPIO_Pin_6 };

#define SCK PAout(4)
#define IO_O PAout(5)
#define IO_I PAin(5)
#define RST PAout(6)


/********************************************************************
* ?? : bcdtodec(uchar bcd)
* ?? : BCD????DEC?
* ?? : bcd?
* ?? : dec?
***********************************************************************/
unsigned char bcdtodec(unsigned char bcd)
{
	unsigned char data1;
	data1 = bcd & 0x0f;     //?BCD?4?
	bcd = bcd & 0x70;       //??BCD??????4??
	data1 += bcd >> 1;
	data1 += bcd >> 3;      //?????????
	return data1;
}


/********************************************************************
* ?? : dectobcd(uchar dec)
* ?? : DEC????BCD?
* ?? : dec?
* ?? : bcd?
***********************************************************************/
unsigned char dectobcd(unsigned char dec)
{
	unsigned char bcd;
	bcd = 0;
	while(dec >= 10)
	{              
		dec -= 10;                         
		bcd++;
	} 
	bcd <<= 4;
	bcd |= dec;
	return bcd;
}


//DS1302?????
void ds1302_init(void) {
	GPIO_QuickInit(gpiozwc_ds1302.SCK_HW_GPIOx, gpiozwc_ds1302.SCK_GPIO_Pin_x , GPIO_Mode_Out_PP);//初始化与继电器连接的硬件接口
	GPIO_QuickInit(gpiozwc_ds1302.RST_HW_GPIOx, gpiozwc_ds1302.RST_GPIO_Pin_x , GPIO_Mode_Out_PP);//初始化与继电器连接的硬件接口
	GPIO_QuickInit(gpiozwc_ds1302.IO_HW_GPIOx, gpiozwc_ds1302.IO_GPIO_Pin_x , GPIO_Mode_Out_PP);//初始化与继电器连接的硬件接口

	RST=0;			 
	SCK=0;			 
}


//?DS1302???????
void ds1302_write_byte(unsigned char addr, unsigned char d){
	unsigned char i;
	IO_o_MODE
	RST=1;					//??DS1302??	
	addr = addr & 0xFE;   
	for (i = 0; i < 8; i ++) {
		if (addr & 0x01) { IO_O=1; }
		else { IO_O=0; }
		SCK=1;    SCK=0;
		addr = addr >> 1;
	}	
	for (i = 0; i < 8; i ++) {
		if (d & 0x01) { IO_O=1; }
		else { IO_O=0; }
		SCK=1;  SCK=0;
		d = d >> 1;
	}
	RST=0;	
}

unsigned char ds1302_read_byte(unsigned char addr) {

	unsigned char i,temp;	
	RST=1;					
	IO_o_MODE
	addr = addr | 0x01;    
	for (i = 0; i < 8; i ++) {
		if (addr & 0x01) { IO_O=1; }
		else { IO_O=0; }
		SCK=1; SCK=0; addr = addr >> 1;
	}	
	IO_i_MODE
	for (i = 0; i < 8; i ++) {
		temp = temp >> 1;
		if (IO_I) { temp |= 0x80; }
		else { 	temp &= 0x7F; }
		SCK=1; SCK=0;
	}	
	RST=0;					
	return temp;
}


//?DS302??????
void ds1302_read_data(unsigned char dat[7])  
{
	dat[0]=bcdtodec (ds1302_read_byte(ds1302_year_add) );		//? 
	dat[1]=bcdtodec (ds1302_read_byte(ds1302_month_add) );		//? 
	dat[2]=bcdtodec (ds1302_read_byte(ds1302_date_add) );		//? 
	dat[3]=bcdtodec (ds1302_read_byte(ds1302_day_add) );		//? 	
	dat[4]=bcdtodec (ds1302_read_byte(ds1302_hr_add) );		//? 
	dat[5]=bcdtodec (ds1302_read_byte(ds1302_min_add) );		//? 
	dat[6]=bcdtodec ((ds1302_read_byte(ds1302_sec_add))&0x7f );//?,?????7?,????59
}



//?DS302??????
void ds1302_write_time(unsigned char year,unsigned char month,unsigned char day,unsigned char week,unsigned char hour,unsigned char min,unsigned char sec) {
	ds1302_init();
	ds1302_write_byte(ds1302_control_add,0x00);	  //????? 
	ds1302_write_byte(ds1302_sec_add,0x80);				//???? 
	//ds1302_write_byte(ds1302_charger_add,0xa9);	//???? 
	ds1302_write_byte(ds1302_year_add,  dectobcd(year));		//? 
	ds1302_write_byte(ds1302_month_add, dectobcd(month));	  //? 
	ds1302_write_byte(ds1302_date_add,  dectobcd(day));		  //? 
	ds1302_write_byte(ds1302_hr_add,    dectobcd(hour));		//? 
	ds1302_write_byte(ds1302_min_add,   dectobcd(min));		  //?
	ds1302_write_byte(ds1302_sec_add,   dectobcd(sec));		  //?
	ds1302_write_byte(ds1302_day_add,   dectobcd(week));		//? 
	ds1302_write_byte(ds1302_control_add,0x80);	  //?????     
}

