/**
  ******************************************************************************
  * @file    Project/STM32F4xx_StdPeriph_Templates/stm32f4xx_it.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    06-March-2015
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "USART.h"
#include "FIFO_USART.h"
#include "Limits.h"
#include "Stepper.h"
#include "System.h"
#include "Settings.h"
#include "Config.h"
#include "MotionControl.h"

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile uint8_t DebounceCounterControl = 0;
volatile uint8_t DebounceCounterLimits = 0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

extern void Limit_PinChangeISR(void);
extern void System_PinChangeISR(void);


void SysTick_Init(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	/* SysTick end of count event each 1ms */
  	RCC_GetClocksFreq(&RCC_Clocks);
  	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

  	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 5));
}


// for 100 MHz STM32F411
#define COUNTS_PER_MICROSECOND	33
void Delay_us(volatile uint32_t us)
{
	volatile uint32_t count = us * COUNTS_PER_MICROSECOND - 2;
	__asm volatile(" mov r0, %[count]  \n\t"
			"1: subs r0, #1            \n\t"
			"   bhi 1b                 \n\t"
			:
			: [count] "r" (count)
			: "r0");
}


void Delay_ms(volatile uint32_t ms)
{
	while(ms--)
		Delay_us(999);
}


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}


/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}


/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}


/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}


/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}


/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	/*
	 * Because of the board layout, we cant attach all pins to interrupts.
	 * Therefore we just poll them in this 1ms task, which is hopefully fast
	 * enough for critical events. Debouncing pins is also implemented here.
	 */
	uint8_t limits = Limits_GetState();
	if(limits) {
		// X-Y-Z Limit
		if((DebounceCounterLimits == 0) && settings.system_flags & BITFLAG_ENABLE_LIMITS) {
			DebounceCounterLimits = 20;
			Limit_PinChangeISR();
		}
	}

	uint8_t controls = System_GetControlState();
	if(controls) {
		// System control
		if(DebounceCounterControl == 0) {
			DebounceCounterControl = 20;
			System_PinChangeISR();
		}
	}

	if(DebounceCounterLimits && !limits) {
		DebounceCounterLimits--;
	}
	if(DebounceCounterControl && !controls) {
		DebounceCounterControl--;
	}
}


/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles TIM9 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM1_BRK_TIM9_IRQHandler(void)
{
	/* TIM9_CH1 */
	if(TIM_GetITStatus(TIM9, TIM_IT_CC1) != RESET) {
		// OC
		Stepper_MainISR();

		TIM_ClearITPendingBit(TIM9, TIM_IT_CC1);
	}
	else if(TIM_GetITStatus(TIM9, TIM_IT_Update) != RESET) {
		// OVF
		Stepper_PortResetISR();

		TIM_ClearITPendingBit(TIM9, TIM_IT_Update);
	}
}


/**
  * @brief  This function handles External lines 9 to 5 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI9_5_IRQHandler(void)
{
}


/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
	}

	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
	}

	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) {
		(void)USART_ReceiveData(USART1);
	}
}


/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		/* Read one byte from the receive data register */
		unsigned char c = (USART_ReceiveData(USART2) & 0xFF);

		// Pick off realtime command characters directly from the serial stream. These characters are
		// not passed into the main buffer, but these set system state flag bits for realtime execution.
		switch(c)
		{
		case CMD_RESET:         MC_Reset(); break; // Call motion control reset routine.
		case CMD_STATUS_REPORT: System_SetExecStateFlag(EXEC_STATUS_REPORT);break;
		case CMD_CYCLE_START:   System_SetExecStateFlag(EXEC_CYCLE_START); break; // Set as true
		case CMD_FEED_HOLD:     System_SetExecStateFlag(EXEC_FEED_HOLD); break; // Set as true
		default:
			if(c > 0x7F) { // Real-time control characters are extended ACSII only.
				switch(c)
				{
				case CMD_SAFETY_DOOR: System_SetExecStateFlag(EXEC_SAFETY_DOOR); break; // Set as true
				case CMD_JOG_CANCEL:
					if(sys.state & STATE_JOG) { // Block all other states from invoking motion cancel.
						System_SetExecStateFlag(EXEC_MOTION_CANCEL);
					}
					break;

				case CMD_FEED_OVR_RESET: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_RESET); break;
				case CMD_FEED_OVR_COARSE_PLUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_COARSE_PLUS); break;
				case CMD_FEED_OVR_COARSE_MINUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_COARSE_MINUS); break;
				case CMD_FEED_OVR_FINE_PLUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_FINE_PLUS); break;
				case CMD_FEED_OVR_FINE_MINUS: System_SetExecMotionOverrideFlag(EXEC_FEED_OVR_FINE_MINUS); break;
				case CMD_RAPID_OVR_RESET: System_SetExecMotionOverrideFlag(EXEC_RAPID_OVR_RESET); break;
				case CMD_RAPID_OVR_MEDIUM: System_SetExecMotionOverrideFlag(EXEC_RAPID_OVR_MEDIUM); break;
				case CMD_RAPID_OVR_LOW: System_SetExecMotionOverrideFlag(EXEC_RAPID_OVR_LOW); break;
				case CMD_SPINDLE_OVR_RESET: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_RESET); break;
				case CMD_SPINDLE_OVR_COARSE_PLUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_COARSE_PLUS); break;
				case CMD_SPINDLE_OVR_COARSE_MINUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_COARSE_MINUS); break;
				case CMD_SPINDLE_OVR_FINE_PLUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_FINE_PLUS); break;
				case CMD_SPINDLE_OVR_FINE_MINUS: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_FINE_MINUS); break;
				case CMD_SPINDLE_OVR_STOP: System_SetExecAccessoryOverrideFlag(EXEC_SPINDLE_OVR_STOP); break;
				case CMD_COOLANT_FLOOD_OVR_TOGGLE: System_SetExecAccessoryOverrideFlag(EXEC_COOLANT_FLOOD_OVR_TOGGLE); break;
#ifdef ENABLE_M7
				case CMD_COOLANT_MIST_OVR_TOGGLE: System_SetExecAccessoryOverrideFlag(EXEC_COOLANT_MIST_OVR_TOGGLE); break;
#endif
				}
			// Throw away any unfound extended-ASCII character by not passing it to the serial buffer.
			}
			else {
				// Write character to buffer
				FifoUsart_Insert(USART2_NUM, USART_DIR_RX, c);
			}
		}
	}

	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		char c;

		if(FifoUsart_Get(USART2_NUM, USART_DIR_TX, &c) == 0) {
			/* Write one byte to the transmit data register */
			while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
			USART_SendData(USART2, c);
		}
		else {
			// Nothing to transmit - disable interrupt
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		}
	}

	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if(USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET) {
		(void)USART_ReceiveData(USART2);
	}
}


/**
  * @brief  This function handles USART6 global interrupt request.
  * @param  None
  * @retval None
  */
void USART6_IRQHandler(void)
{
	if(USART_GetITStatus(USART6, USART_IT_RXNE) != RESET) {
		/* Read one byte from the receive data register */
		unsigned char c = (USART_ReceiveData(USART6) & 0xFF);

		// Write character to buffer
		FifoUsart_Insert(USART6_NUM, USART_DIR_RX, c);
	}

	if(USART_GetITStatus(USART6, USART_IT_TXE) != RESET) {
		char c;

		if(FifoUsart_Get(USART6_NUM, USART_DIR_TX, &c) == 0) {
			/* Write one byte to the transmit data register */
			while(USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET);
			USART_SendData(USART6, c);
		}
		else {
			// Nothing to transmit - disable interrupt
			USART_ITConfig(USART6, USART_IT_TXE, DISABLE);
		}
	}

	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if(USART_GetFlagStatus(USART6, USART_FLAG_ORE) != RESET) {
		(void)USART_ReceiveData(USART6);
	}
}

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/