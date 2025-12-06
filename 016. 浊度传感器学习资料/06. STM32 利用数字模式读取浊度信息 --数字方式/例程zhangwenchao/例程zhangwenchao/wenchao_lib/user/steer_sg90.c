#include "steer_sg90.h"

/*
 * PA8  @ TIM1 CH1
 * PB13 @ TIM1 CH1N
 * PA9  @ TIM1 CH2
 * PB14 @ TIM1 CH2N
 * PA10 @ TIM1 CH3
 * PA11 @ TIM1 CH4
 * PA0  @ TIM2 CH1
 * PA1  @ TIM2 CH2
 * PA2  @ TIM2 CH3
 * PA3  @ TIM2 CH3
 * PA6  @ TIM3 CH1
 * PA7  @ TIM3 CH2
 * PB0  @ TIM3 CH3
 * PB1  @ TIM3 CH4
 * PB6  @ TIM4 CH1
 * PB7  @ TIM4 CH2
 * PB8  @ TIM4 CH3
 * PB9  @ TIM4 CH4
*/

void steer_sg90_init(TIM_TypeDef* TIMx, u8 PWM_CHx){
	TIM_PWM_Init(TIMx, PWM_CHx, 50, ENABLE);
}


void steer_sg90_run_to_angle(TIM_TypeDef* TIMx, u8 PWM_CHx, u8 angle){
		TIM_SetCompare(TIMx, PWM_CHx, 5000/20 + 1000*angle/180);
}

