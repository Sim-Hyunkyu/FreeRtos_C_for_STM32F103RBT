/*
 * user_hal_uart.c
 *
 *  Created on: Nov 25, 2016
 *      Author: Kevin
 */


#include <stm32f1xx_hal.h>

#include "main.h"
#include "Uart1Handler.h"
#include "Uart3Handler.h"
#include "PrintTask.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint16_t* size;
	uint8_t* info;
	uint8_t* msg = huart->pRxBuffPtr;
	// Locate the begining of the message to send to the task.
	msg -= huart->RxXferSize;  // Size was original
	msg += huart->RxXferCount; // Count is how much space is left
	info = msg - 3;
	size = (uint16_t*) (info + 1);
	*size = huart->RxXferSize - huart->RxXferCount;

	// Callback for the uart
	if (huart->Instance == USART1)
	{
		Uart1HandlerPost((uint32_t)info, 0); // Timeout must be zero
	}
	else if (huart->Instance == USART2)
	{
		DebugInputPost((uint32_t)info, 0); // Timeout must be zero
	}
	else if (huart->Instance == USART3)
	{
		Uart3HandlerPost((uint32_t)info, 0);
	}
}
