#include "fsl_device_registers.h"

/* For a two floors j = 140
 * For one floor j = 68 */

#define lvlOne 0x0004; 		// pin 2
#define lvlTwo 0x0008; 		// pin 3
#define lvlThree 0x0400; 	// pin 10
#define alarm 0x0800; 		// pin 11, PORTB

unsigned int flag = 0;

unsigned int i;
unsigned int btn1, btn2, btn3, stopBtn;
unsigned int level, lastFloor, temp, elevDir;
unsigned int input;
unsigned int alertLED;

unsigned int currLevel = 1;
unsigned int stop1 = 0;
unsigned long speed = 0, adc_data = 0; voltage = 0;
//For 7-Segment Display
unsigned char decodePortC[10] = {0x30, 0xAD, 0xB9};

/* Helper Functions Used */
void PORTA_IRQHandler(void);
void delayby1ms(int k);
unsigned short ADC_read16b(void);

unsigned short ADC_read16b(void){
	ADC0_SC1A = 0x00; //ADC0_DPO pin
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); //Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK)); //Until conversion complete
	return ADC0_RA; //ADC conversion result for ADC0
}

int levelsToGo;
unsigned int iterations = 1;
void levelFunction(int lev, unsigned long speed){
	levelsToGo = lev - (int)currLevel;// 3 - 1
	if(levelsToGo == 2){
		for(int i=0; i < 2; i++){
			for(int j=0; j<70; j++){
				GPIOD_PDOR = 0x3A;
				delayby1ms(speed);
				GPIOD_PDOR = 0x39;
				delayby1ms(speed);
				GPIOD_PDOR = 0x35;
				delayby1ms(speed);
				GPIOD_PDOR = 0x36;
				delayby1ms(speed);
				GPIOD_PDOR = 0x00;
				delayby1ms(speed);
			}
			GPIOC_PDOR = decodePortC[1];
		}
		currLevel = 3;
		GPIOC_PDOR = decodePortC[2];
	}
	else if(levelsToGo == 1){//2 - 1
		for(int i=0; i < 1; i++){
			for(int j=0; j<70; j++){
				GPIOD_PDOR = 0x3A;
				delayby1ms(speed);
				GPIOD_PDOR = 0x39;
				delayby1ms(speed);
				GPIOD_PDOR = 0x35;
				delayby1ms(speed);
				GPIOD_PDOR = 0x36;
				delayby1ms(speed);
				GPIOD_PDOR = 0x00;
				delayby1ms(speed);
			}
		}
		if(currLevel == 2){// 2->3
			currLevel = 3;
			GPIOC_PDOR = decodePortC[2];
		} else if(currLevel == 1){//1->2
			currLevel = 2;
			GPIOC_PDOR = decodePortC[1];
		}
	}
	else if(levelsToGo == -1){// 1->2 or 2->3
		for(int j = 0; j <70; j++){
			GPIOD_PDOR = 0x36;
			delayby1ms(speed);
			GPIOD_PDOR = 0x35;
			delayby1ms(speed);
			GPIOD_PDOR = 0x39;
			delayby1ms(speed);
			GPIOD_PDOR = 0x3A;
			delayby1ms(speed);
		}
		if(currLevel == 3){
			currLevel = 2;
			GPIOC_PDOR = decodePortC[1];
		}else if(currLevel == 2){
			currLevel = 1;
			GPIOC_PDOR = decodePortC[0];
		}
	}
	else if(levelsToGo == -2){
		for(int i = 0; i < 2; i++){
			for(int j = 0; j <70; j++){
				GPIOD_PDOR = 0x36;
				delayby1ms(speed);
				GPIOD_PDOR = 0x35;
				delayby1ms(speed);
				GPIOD_PDOR = 0x39;
				delayby1ms(speed);
				GPIOD_PDOR = 0x3A;
				delayby1ms(speed);
			}
			GPIOC_PDOR = decodePortC[1];
		}
		GPIOC_PDOR = decodePortC[0];
		currLevel = 1;
	}
}
/* Helper Functions Used */
void PORTA_IRQHandler(void){
	NVIC_ClearPendingIRQ(PORTA_IRQn); //Clear pending interrupts
	flag = 1;
	while(flag == 1){
		stopBtn = GPIOB_PDIR & alarm; // pin 11 (emergency)
		if(stopBtn == 0){
			GPIOD_PDOR ^= 1 << 6; //Toggle LED on
			GPIOD_PDOR = 0x80; //Buzzer noise
		}
		else{
			flag = 0;
		}
	}

	PORTA_ISFR=(1<<1); //Clear ISFR for PORTA, Pin 1
}
// Buzzer noise
void alarmSound(){
	for(i=0;i<80;i++){
		GPIOD_PDOR = 0x80;
		delayby1ms(5);
		GPIOD_PDOR = 0x00;
		delayby1ms(5);
	}
	for(i=0;i<100;i++){
		GPIOD_PDOR = 0x80;
		delayby1ms(10);
		GPIOD_PDOR = 0x00;
		delayby1ms(10);
	}
}

void delayby1ms(int k){
	FTM3_C6SC = 0x1C;
	FTM3_C6V = FTM3_CNT + 333;

	for (int i = 0; i < k; i++){
		while(!(FTM3_C6SC & 0x80));
		FTM3_C6SC &= ~(1 << 7);
		FTM3_C6V = FTM3_CNT + 333;
	}
	FTM3_C6SC = 0;
}
int main(void){

	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK; //Enable Port A Clock Gate Control
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; // Enable Port B Clock Gate Control
	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK; // Enable Port C Clock Gate Control
	SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; // Enable Port D Clock Gate Control

	SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK; // Enable ADC Clock Gate Control

	PORTB_GPCLR = 0x0C0C0100; //Configure PORTB[11:10,3:2] for GPIO
	PORTD_GPCLR = 0x00FF0100; //Configure PORTD[7:0] for GPIO
	PORTC_GPCLR = 0x01BF0100; //Configure PORTC[8:7, 5:0] for GPIO

	GPIOB_PDDR = 0x00000000; // Configure PORTB[11:10,3:2] for Input
	GPIOC_PDDR = 0x000001BF; // Configure PORTC[8:7, 5:0] for Output
	GPIOD_PDDR = 0x000000FF; // Configure PORTD[7:0] for Output

	ADC0_CFG1 = 0x0C; //16 bits ADC; Bus Clock
	ADC0_SC1A = 0x1F; //Disable the module, ADCH = 11111

	PORTA_PCR1 = 0xC0100; // Configure Port A pin 1 for GPIO and interrupt when logic is high.
	PORTA_ISFR = (1<<1); // Clear ISFR for Port A, Pin 1.

	// Initialization for timer function
	SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK;
	FTM3_MODE = 0x5;
	FTM3_MOD = 0xFFFF;
	FTM3_SC = 0x0E;

	GPIOC_PDOR = decodePortC[0]; // displays '1' on 7-segment display
	while(1){
		//Interrupt
		NVIC_EnableIRQ(PORTA_IRQn); //Enable interrupts from Port A, Pin 1
		GPIOD_PDOR = 0x00;

		input = GPIOB_PDIR & 0x0C0C;
		//		btn1 = GPIOB_PDIR & lvlOne //pin 2 (floor1)
		//		btn2 = GPIOB_PDIR & lvlTwo; //pin 3 (floor2)
		//		btn3 = GPIOB_PDIR & lvlThree; // pin 10 (floor3)
		//		stopBtn = GPIOB_PDIR & alarm; // pin 11 (emergency)

		adc_data = ADC_read16b();
		voltage = (adc_data*3.3)/(65535); //3.3 = V_RH , 65535

		if(voltage >= 0 && voltage <= 1 ){
			speed = 50;
		}
		else if(voltage > 1 && voltage <= 2){
			speed = 20;
		}
		else{
			speed = 5;
		}

		int a = 3;
		int b = 1;
		int c = b - a;
		if(input & 0x0004){
			level = 3;
			levelFunction(level, speed);
		}
		else if(input & 0x0008){
			level = 2;
			levelFunction(level,speed);
		}
		else if(input & 0x0400){
			level = 1;
			levelFunction(level,speed);
		}
	}
}
