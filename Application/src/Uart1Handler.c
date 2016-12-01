/*
 * AppThread.cpp
 *
 *  Created on: Nov 20, 2016
 *      Author: Kevin Brown
 *
 *  Example Thread
 */

#include <stdio.h>
#include <string.h>
#include <osMalloc.h>
#include "cmsis_os.h"
#include <stm32f1xx_hal.h>
#include <Uart1Handler.h>
#include "PrintTask.h"
#include "main.h"

static void ThreadExec(const void *args);
static UART_HandleTypeDef huart1;

// Task Variables
// Make them all static to limit their scope to this module
// If access is required by other threads interface functions to the data
// must be created and a mutex must be used for all set and get operations.

static osThreadDef(uart1HandlerTask, ThreadExec, osPriorityHigh, 0, 512);
static osThreadId uart1TaskHandle;

static osMessageQId uart1mailbox;
static const osMessageQDef_t uart1MessagDef =
{ 16,
  sizeof (uint32_t)
};


static int runflag = 1;

/*******************************************************************
* USART1_IRQHandler()                                              *
* This is the ISR handler that replaces the __weak definition      *
* defined by the HAL. It calls the HAL_UART_IRQ handler with the   *
* uart handle.                                                     *
*******************************************************************/

void USART1_IRQHandler()
{
  if(USART1->SR & USART_SR_RXNE)
  {
	  HAL_UART_IRQHandler(&huart1);
  }
}

// Private Functions used only by this Thread

/*******************************************************************
* Setup() Setup function contains initializations needed by the    *
*         thread. Just prior to starting the loop.                 *
*******************************************************************/
static void Setup()
{
	__HAL_UART_CLEAR_PEFLAG(&huart1);
	NVIC_EnableIRQ(USART1_IRQn);
	// Whatever you want to add prior to loop starting
	// The constructor can also be used but
	// Setup occurs after constructor but prior to the first call of
	// Loop
}

/*******************************************************************
* CleanUp() Contains code needed by the thread to clean up any     *
* allocations or locked resources. Called when the thread loop     *
* exits.                                                           *
*******************************************************************/
static void CleanUp()
{
	// Perform Cleanup operations for thread termination
}

static uint8_t* dataQueued = NULL;
static uint8_t dataLength = 0;

/*******************************************************************
* QueueuUpNext(maxLen)                                             *
* The ISR for the UART requires the user supply a storage location *
* and a size of that store location.   This function will osMalloc *
* that storage which will be posted to the task when it is full or *
* an end of line marker is encountered.                            *
*******************************************************************/
static void QueueUpNext(int maxLen)
{
	HAL_StatusTypeDef status;
	dataLength = maxLen;
	dataQueued = (uint8_t*) osMalloc(maxLen+3);
	if (dataQueued != NULL)
	{
		uint16_t *size = (uint16_t*) (dataQueued+1);
		memset(dataQueued, 0, dataLength+3);
		dataQueued[0] = CMD_ISR_DATA;
		*size = 0;
		huart1.ErrorCode = 0;
		__HAL_UART_CLEAR_PEFLAG(&huart1);
		status = HAL_UART_Receive_IT(&huart1, dataQueued+3, maxLen);
		if (status != HAL_OK)
		{
			// TBD: Handle a failure of the UART Receive IT fault.
		}
	}
}

/*******************************************************************
* ReQueueData()                                                    *
* If the UART seems to stall this function is used to requeue that *
* last buffer to the ISR. This will reset all of the pointers and  *
* offsets.                                                         *
*******************************************************************/
static void ReQueueData(void)
{
	HAL_StatusTypeDef status;
	uint16_t *size = (uint16_t*)(dataQueued+1);
	memset(dataQueued, 0, dataLength+3);
	dataQueued[0] = CMD_ISR_DATA;
	*size = 0;

	huart1.ErrorCode = 0;
	__HAL_UART_CLEAR_PEFLAG(&huart1); // Clears any errors
	// Send ISR the data buffer which is 3 bytes past allocation
	status = HAL_UART_Receive_IT(&huart1, dataQueued+3, dataLength);
	if (status != HAL_OK)
	{

	}
}


/*******************************************************************
* ThreadExec(args) the Executive Loop                              *
*    Threads should spend most of the time delayed.                *
*******************************************************************/
static void ThreadExec(const void *args)
{
	int maxSize = 80;
	Setup();
	QueueUpNext(maxSize);
	while (runflag)
	{
		osEvent event = osMessageGet(uart1mailbox, 2000); // 1.5 seconds
		if (event.status != osEventTimeout)
		{
			char* msg = (char*) event.value.v;
			if (msg != NULL)
			{
				uint8_t cmd = msg[0];
				uint16_t size = *((uint16_t*) (msg+1));
				char *data = msg+3;

				QueueUpNext(maxSize);
				// Process Message
				switch(cmd)
				{
					case CMD_ISR_DATA:
						// Handle data from ISR
						xprintf("1: (%d) %s\r\n", size, data);
						break;
					case CMD_CONTROL_INFO:
						break;
				}

				// Pointer must be freed
				// However if you want to pass this buffer
				// To another task don't free it here,
				// Post it instead to the other task
				osFree(msg);
			}
		}
		else
		{
//			ReQueueData(); // Perform this if no longer receiving data
		}
	}
    // Thread is terminating so perform cleanup and delete the task

	CleanUp();

	osThreadTerminate(uart1TaskHandle);
}

static uint16_t DoneOnCR(void* handle)
{
	uint16_t retval = 0;
	UART_HandleTypeDef* pUartH = (UART_HandleTypeDef*) handle;

	if (*(pUartH->pRxBuffPtr-1) == 0x0a) // Return after a Carriage Return
		retval = 1;

	return retval;
}

/* USART1 init function */
static void MX_USART1_UART_Init(uint32_t rate)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = rate;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.pUartDone = DoneOnCR;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
//    Error_Handler();
  }
}

// Public Functions are defined here

/*******************************************************************
* Uart1ThreadPost()                                                *
* Used to post data to the task.                                   *
* Interface can be designed to the tasks requirements.             *
* Note: ISRs can post but the TmoMS must be zero for an ISR to use *
* it.                                                              *
*******************************************************************/
int Uart1HandlerPost(uint32_t msg, uint32_t TmoMs)
{
	int retval = 0;
	osStatus status;
	// Post a command for the task with data
	status = osMessagePut(uart1mailbox, msg, TmoMs);
	if (status == osOK)
	{
		retval = 1;
	}

	return retval;
}

/*******************************************************************
* Uart2HandlerCreate()                                             *
* Performs initialization for the tasks.  All OS initializations   *
* need to be done in this task.  It will be called from main().    *
*******************************************************************/
void Uart1HandlerCreate()
{
	// Perform all Thread OS Initializations: Task, MailQ, Semphores, etc
	  runflag = 1;
	  MX_USART1_UART_Init(9600);
	  uart1TaskHandle = osThreadCreate(osThread(uart1HandlerTask), NULL);
	  uart1mailbox = osMessageCreate(&uart1MessagDef, uart1TaskHandle);
}

/*******************************************************************
* DeleteUart2Handler()                                             *
* Used to stop a task.   It will stop at the next itteration thru  *
* the loop.                                                        *
*******************************************************************/
void DeleteUart1Handler()
{
	runflag = 0;
}
