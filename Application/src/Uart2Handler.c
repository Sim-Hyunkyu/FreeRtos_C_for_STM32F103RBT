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
#include <Uart2Handler.h>
#include "PrintTask.h"
#include "main.h"

typedef struct
{
	uint32_t command; // Command to be used by the task to interpret the data
	uint32_t data0;   // Two Parameters 32bit value can be used as information
	uint32_t data1;   //     or as pointers to data.
} MAILBOX_STRUCT;

static void ThreadExec(const void *args);
static UART_HandleTypeDef huart2;

// Task Variables
// Make them all static to limit their scope to this module
// If access is required by other threads interface functions to the data
// must be created and a mutex must be used for all set and get operations.

static osThreadDef(uart2HandlerTask, ThreadExec, osPriorityHigh, 0, 512);
static osThreadId uart2TaskHandle;

static osMessageQId uart2mailbox;
static const osMessageQDef_t uart2MessagDef =
{ 16,
  sizeof (uint32_t)
};


static int runflag = 1;

#if 0
/*******************************************************************
* USART2_IRQHandler()                                              *
* This is the ISR handler that replaces the __weak definition      *
* defined by the HAL. It calls the HAL_UART_IRQ handler with the   *
* uart handle.                                                     *
*******************************************************************/

void USART2_IRQHandler()
{
  if(USART2->SR & USART_SR_RXNE)
  {
	  HAL_UART_IRQHandler(&huart2);
  }
}
#endif

// Private Functions used only by this Thread

/*******************************************************************
* Setup() Setup function contains initializations needed by the    *
*         thread. Just prior to starting the loop.                 *
*******************************************************************/
static void Setup()
{
	__HAL_UART_CLEAR_PEFLAG(&huart2);
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
		huart2.ErrorCode = 0;
		__HAL_UART_CLEAR_PEFLAG(&huart2);
		status = HAL_UART_Receive_IT(&huart2, dataQueued+3, maxLen);
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

	huart2.ErrorCode = 0;
	__HAL_UART_CLEAR_PEFLAG(&huart2); // Clears any errors
	// Send ISR the data buffer which is 3 bytes past allocation
	status = HAL_UART_Receive_IT(&huart2, dataQueued+3, dataLength);
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
	Setup();

	QueueUpNext(80);
	while (runflag)
	{
		osEvent event = osMessageGet(uart2mailbox, 2000); // 1.5 seconds
		if (event.status == osEventMessage)
		{
			char* msg = (char*) event.value.v;
			if (msg != NULL)
			{
				uint8_t cmd = msg[0];
				uint16_t size = *((uint16_t*) (msg+1));
				char *data = msg+3;

				QueueUpNext(80);
				// Process Message
				/// Processing here!!
				switch(cmd)
				{
					case CMD_ISR_DATA:
						// Handle data from ISR
						xprintf("(%d) %s\n", size, data);
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

	osThreadTerminate(uart2TaskHandle);
}

static uint16_t DoneOnCR(void* handle)
{
	uint16_t retval = 0;
	UART_HandleTypeDef* pUartH = (UART_HandleTypeDef*) handle;

	if (*(pUartH->pRxBuffPtr-1) == 0x0D) // Return after a Carriage Return
		retval = 1;

	return retval;
}

/* USART1 init function */
static void MX_USART2_UART_Init(uint32_t rate)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = rate;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.pUartDone = DoneOnCR;
  if (HAL_UART_Init(&huart2) != HAL_OK)
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
int Uart2HandlerPost(uint32_t msg, uint32_t TmoMs)
{
	int retval = 0;
	osStatus status;
	// Post a command for the task with data
	status = osMessagePut(uart2mailbox, msg, TmoMs);
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
void Uart2HandlerCreate()
{
	// Perform all Thread OS Initializations: Task, MailQ, Semphores, etc
	  runflag = 1;
	  MX_USART2_UART_Init(115200);
	  uart2TaskHandle = osThreadCreate(osThread(uart2HandlerTask), NULL);
	  uart2mailbox = osMessageCreate(&uart2MessagDef, uart2TaskHandle);
}

/*******************************************************************
* DeleteUart2Handler()                                             *
* Used to stop a task.   It will stop at the next itteration thru  *
* the loop.                                                        *
*******************************************************************/
void DeleteUart2Handler()
{
	runflag = 0;
}
