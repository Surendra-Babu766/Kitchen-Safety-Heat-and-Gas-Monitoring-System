#include <lpc21xx.h>
#include "types.h"
#include "KPM.h"
#include "LCD.h"
#include "LCD_defines.h"
#include "delay.h"
#include <string.h>
#define EINT0_CHNO 14
#define EINT1_CHNO 15
//allert led
#define EINT0_LED 0
//buzzer pin
#define BUZ_PIN 27
volatile u32 flag=0;
volatile u32 edit_flag;
//isr for turning off LED and BUZZER
void eint0_isr(void)__irq{
	flag=1;
	IOCLR0=1<<EINT0_LED;
	IOCLR1=1<<BUZ_PIN;
	EXTINT=1<<0;
	VICVectAddr=0;
	
}

void eint0_enable(void){
	
	//cfg p0.1 as EXTINT0
  
		PINSEL0 &= ~(3 << (1*2));
		PINSEL0 |=  (3 << (1*2));
	//select EXTINT0 as IRQ
		VICIntSelect|=0<<EINT0_CHNO;
 //enable extint0 intreeupt source
	VICIntEnClr|=1<<EINT0_CHNO;
	//load isr address
	VICVectAddr0=(u32)eint0_isr;
	//select slot for extint0
	VICVectCntl0=(1<<5)|EINT0_CHNO;
	//select edge trigggering
	EXTMODE=1<<0;
	EXTPOLAR = 0;
	//enable extint0 intreeupt source
	VICIntEnable|=1<<EINT0_CHNO;
	
}

//isr for EDIT MODE
void eint1_isr(void)__irq{
	edit_flag=1;
	EXTINT=1<<1;
	VICVectAddr=0;
	
}

//enabling the switch1 for EDIT MODE
void eint1_enable(void){
	
	//cfg p0.3 as EXTINT1
  
		PINSEL0 &= ~(3 << (3*2));
		PINSEL0 |=  (3 << (3*2));
	//select EXTINT1 as IRQ
		VICIntSelect &= ~(1 << EINT1_CHNO);
 //enable extint0 intreeupt source
	VICIntEnClr|=1<<EINT1_CHNO;
	//load isr address
	VICVectAddr1=(u32)eint1_isr;
	//select slot for extint1
	VICVectCntl1=(1<<5)|EINT1_CHNO;
	//select edge trigggering
	EXTMODE=1<<0;
	EXTPOLAR = 0;
	//enable extint1 intreeupt source
	VICIntEnable|=1<<EINT1_CHNO;
	
}

