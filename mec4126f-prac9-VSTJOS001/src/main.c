// Description----------------------------------------------------------------|
/*
 * Initialises a struct with Name and Age data. Displays results on LEDs and
 * LCD.
 */
// DEFINES AND INCLUDES-------------------------------------------------------|

#define STM32F051
//>>> Uncomment line 10 if using System Workbench (SW4STM32) or STM32CubeIDE
#define SW4STM32

#ifndef SW4STM32
	#define TRUESTUDIO
#endif

#include "stm32f0xx.h"

// GLOBAL VARIABLES ----------------------------------------------------------|
uint16_t duty_cycle;

// FUNCTION DECLARATIONS -----------------------------------------------------|

void main(void);                                                   //COMPULSORY
void init_ADC(void);											   //COMPULSORY
void init_timer_2(void);
void init_timer_6(void);
void TIM6_DAC_IRQHandler(void);
#ifdef TRUESTUDIO												   //COMPULSORY
	void reset_clock_to_48Mhz(void);							   //COMPULSORY
#endif															   //COMPULSORY



// MAIN FUNCTION -------------------------------------------------------------|

void main(void)
{
#ifdef TRUESTUDIO  											 	   //COMPULSORY
	reset_clock_to_48Mhz();										   //COMPULSORY
#endif															   //COMPULSORY
	init_timer_2();
	init_timer_6();

	while(1)
	{

	}
}

// OTHER FUNCTIONS -----------------------------------------------------------|

#ifdef TRUESTUDIO												   //COMPULSORY
/* Description:
 * This function resets the STM32 Clocks to 48 MHz
 */
void reset_clock_to_48Mhz(void)									   //COMPULSORY
{																   //COMPULSORY
	if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL)			   //COMPULSORY
	{															   //COMPULSORY
		RCC->CFGR &= (uint32_t) (~RCC_CFGR_SW);					   //COMPULSORY
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);	   //COMPULSORY
	}															   //COMPULSORY

	RCC->CR &= (uint32_t)(~RCC_CR_PLLON);						   //COMPULSORY
	while ((RCC->CR & RCC_CR_PLLRDY) != 0);						   //COMPULSORY
	RCC->CFGR = ((RCC->CFGR & (~0x003C0000)) | 0x00280000);		   //COMPULSORY
	RCC->CR |= RCC_CR_PLLON;									   //COMPULSORY
	while ((RCC->CR & RCC_CR_PLLRDY) == 0);						   //COMPULSORY
	RCC->CFGR |= (uint32_t) (RCC_CFGR_SW_PLL);					   //COMPULSORY
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);		   //COMPULSORY
}																   //COMPULSORY
#endif															   //COMPULSORY

void init_ADC(void)
{
	//
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	//
	GPIOA->MODER |= GPIO_MODER_MODER6;
	//
	RCC->APB2ENR |= RCC_APB2ENR_ADCEN;
	//
	ADC1->CR |= ADC_CR_ADEN;
	//
	ADC1->CHSELR |= ADC_CHSELR_CHSEL6;
	// Setup in wait mode
	ADC1->CFGR1 &= ~ADC_CFGR1_CONT;
	// Setup 10 bit resolution
	ADC1->CFGR1 |= ADC_CFGR1_RES_0;
	//
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0);
}

void init_timer_2(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;     //enable clock to Timer 2
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;      //enable clock to PB
	GPIOB->MODER |= GPIO_MODER_MODER10_1;   //set B10 to alternate function
	GPIOB->MODER |= GPIO_MODER_MODER11_1;   //set B11 to alternate function
	GPIOB->AFR[1] |= 0b1000000000;          //set AFR to AFR10 for B10
	GPIOB->AFR[1] |= 0b10000000000000;      //set AFR to AFR11 for B11

	TIM2->PSC = 3;                          //ARR < 2^16
	TIM2->ARR = 1023;                       //(1/15000)/(1/48000000)
	TIM2->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE; //configure PWM
	TIM2->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE; //configure PWM
	TIM2->CCER |= TIM_CCER_CC3E;                                          //Enabling CC Output
	TIM2->CCER |= TIM_CCER_CC4E;                                          //Enabling CC Output
	TIM2->CR1 |= TIM_CR1_CEN;                                             //Enable Timer 2
	TIM2->CCR3 = duty_cycle;                //duty cycle for B10 controlled by pot A6
	TIM2->CCR4 = (3*1023)/4;                //75% duty cycle

}

void init_timer_6(void)
{
	//Enable Timer 6 clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
	//Setting up delay for interrupt
	TIM6->PSC = 36;
	TIM6->ARR = 66667;
	//Allow overflow interrupts
	TIM6->DIER |= TIM_DIER_UIE;
	//Enable counter for Timer 6, and auto-reload preload enable
	TIM6->CR1 |= TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

// INTERRUPT HANDLERS --------------------------------------------------------|

void TIM6_DAC_IRQHandler(void)
{
	TIM6->SR &= ~TIM_SR_UIF;	//acknowledge interrupt
	ADC1->CR |= ADC_CR_ADSTART;
	while((ADC1->ISR & ADC_ISR_EOC) == 0);
	duty_cycle = ADC1->DR;
}
