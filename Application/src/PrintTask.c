/*
 * PrintTask.c
 *
 *  Created on: Nov 20, 2016
 *      Author: Kevin Brown
 *
 *  Example Thread Interface
 */

#include <stdarg.h>
#include <osMalloc.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stm32f1xx_hal.h>
// #include <stm32f1xx_hal_wwdg.h>
#include "cmsis_os.h"
#include "main.h"
#include "PrintTask.h"
#include "flash_if.h"

static void PrintThreadExec(const void *args);
static osThreadDef(PrintTask, PrintThreadExec, osPriorityLow, 0, 256);
static osThreadId printTaskHandle;
static osMessageQId printMsgQid;
static const osMessageQDef_t printMsgDefn =
{ 28,
  sizeof (uint32_t)
};

static UART_HandleTypeDef huart2;
// yprintf used only by PrintTask
// Prints immediately and prevents queuing its own print
static int yprintf(char* fmt, ...);
static int printOn = 1;

static int runflag = 1;
const CONFIG_STRUCT configDefault = {
		"iot.eclipse.org",
		1883,
		"wholesale",
		"hhiyyT-bSGYGuyiz",
		"Pkc7YWjbbGjgt_ZW",
		"U7qTNFuLwHEPkvW4",
		"aGhpeXlULWJTR1lHdXlpejpQa2M3WVdqYmJHamd0X1pX",
		10
};

CONFIG_STRUCT configStorePhysical = {
		"iot.eclipse.org",
		1883,
		"wholesale",
		"hhiyyT-bSGYGuyiz",
		"Pkc7YWjbbGjgt_ZW",
		"U7qTNFuLwHEPkvW4",
		"aGhpeXlULWJTR1lHdXlpejpQa2M3WVdqYmJHamd0X1pX",
		10
};
CONFIG_STRUCT* configStore = &configStorePhysical;


static void ToUpper(char* str)
{
	int i;
	for (i = 0; i<strlen(str); i++)
		str[i] = (char) toupper((int)str[i]);
}

static char *Trim(char *str)
{
    char *end;

    while (isspace(*str))
        str++;

    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;

    *(++end) = '\0';

    return str;
}

static int Split(char *str, char delim, char *tokv[], int maxtoks)
{
    int tokc = 0;
    char *loc;

    while (tokc < maxtoks && (loc = strchr(str, delim)) != NULL)
    {
        *loc = '\0';
        *tokv++ = Trim(str);
        tokc++;
        str = loc + 1;
    }

    if (tokc < maxtoks)
    {
        *tokv++ = Trim(str);
        tokc++;
    }

    return tokc;
}

// static WWDG_HandleTypeDef hwwdg;

#if 0
/* WWDG init function */
static void MX_WWDG_Init(void)
{

  hwwdg.Instance = WWDG;
  hwwdg.Init.Prescaler = WWDG_PRESCALER_1;
  hwwdg.Init.Window = 64;
  hwwdg.Init.Counter = 64;
  if (HAL_WWDG_Init(&hwwdg) != HAL_OK)
  {
    Error_Handler();
  }

}

#endif

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
static void Edit(char* str)
{
	int i = 0;
	int len = strlen(str);
	do
	{
		if (str[i] == '\b') // backspace
		{
			// Move everything from the next character to the
			// character that the bs is trying to delete
			memcpy(&str[i-1], &str[i+1], strlen(&str[i+1])+1);
			len = strlen(str); // Reset Length
			i -= 2; // back up 2 one for deleted character and one for \b
		}
		i++;
	} while (i<len);
}

static void DisplayConfig()
{
	yprintf("\r\n\r\n");
	yprintf("APN:           %s\r\n", configStore->apn);
	yprintf("HOST:          %s\r\n", configStore->host);
	yprintf("PORT:          %d\r\n", configStore->port);
	yprintf("USERNAME:      %s\r\n", configStore->username);
	yprintf("PASSWORD:      %s\r\n", configStore->password);
	yprintf("THING TOKEN:   %s\r\n", configStore->thing_token);
	yprintf("ACCOUNT TOKEN: %s\r\n", configStore->account_token);
	yprintf("REPORT RATE:   %d\r\n", configStore->report_rate);
	yprintf("\r\n");
}

int FlashBlank()
{
	int retval = 1;
	int i;
	uint8_t* data = (void*)USER_NV_STORE;
	for (i = 0; i<sizeof(CONFIG_STRUCT); i++)
	{
		if (data[i] != 0xFF)
		{
			retval = 0;
			break;
		}

	}
	return retval;
}

void LoadConfigFromFlash()
{
	if (FlashBlank())
	{
		memcpy(&configStorePhysical, (CONFIG_STRUCT*)&configDefault, sizeof(CONFIG_STRUCT)) ;
	}
	else
	{
		memcpy(&configStorePhysical, (CONFIG_STRUCT*)USER_NV_STORE, sizeof(CONFIG_STRUCT)) ;
	}
}
static void DumpMem(char *Disp, unsigned char *pBuff, int size)
{
        int i, j;
        long Address = (int) pBuff;
        int line = 0;
static char pucMessage[100];
        int iCnt;
        int iPosn = 0;
        int iMax = sizeof(pucMessage);


	yprintf("\r\n%s [%d] \n", Disp, size);

	for (i = 0; i<size;)
	{
		iPosn = 0;
		for (j = 0; ((i+j) < size) && (j<16); j++) {
				if (((i+j) % 16) == 0) {
						iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "\r\n%04lx: ", (long) Address+j);
						iPosn += iCnt;
				}
				iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "%02x ", pBuff[i+j]);
				iPosn += iCnt;
		}
		for (; j<16; j++) {
				iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "   ");
				iPosn += iCnt;
		}
		iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "  ");
		iPosn += iCnt;
		for (j = 0; ((i+j) < size) && (j<16); j++) {
				iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "%c ", isprint((int) pBuff[i+j]) ? pBuff[i+j] : '.');
				iPosn += iCnt;
		}

		yprintf(pucMessage);

		i+=16;
		Address+=16;
		line++;
	#if 0
		if (factor == 0) {
			if (line == 20) {
				factor = (lines - 1 - line);
				i+=(16*factor);
				Address+=(16*factor);
				line+=factor;
				printf("\r\n.\r\n.\r\n.");
			}
		}
#endif
	}
	yprintf("\r\n\n");
}


static uint8_t nvBuffer[0x400];

#define MAX_PARAMS 10
static char* params[MAX_PARAMS];
/*******************************************************************
* ProcessCommand(data)                                             *
* Handle user commands.                                            *
*******************************************************************/
void ProcessCommand(char* data)
{
	int paramCnt;
	Edit(data); // Allow for backspace
	paramCnt = Split(data, ' ', params, MAX_PARAMS);
	if (paramCnt == 10)
	{
		// Possible Parameter overage
	}
	ToUpper(params[0]);
	if (strcmp(params[0], "SET") == 0)
	{
		ToUpper(params[1]);
		if (strcmp(params[1], "APN") == 0)
		{
			// Set APN
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (strlen(params[2]) > sizeof(configStore->apn)-1)
			{
				yprintf("Not Set: %s max length %d\r\n", params[1], sizeof(configStore->apn)-1);
			}
			else
			{
				memset(configStore->apn, 0, sizeof(configStore->apn));
				strcpy(configStore->apn, params[2]);
			}
		}
		else if (strcmp(params[1], "HOST") == 0)
		{
			// set Host URL
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (strlen(params[2]) > sizeof(configStore->host)-1)
			{
				yprintf("Not Set: %s max length %d\r\n", params[1], sizeof(configStore->host)-1);
			}
			else
			{
				memset(configStore->host, 0, sizeof(configStore->host));
				strcpy(configStore->host, params[2]);
			}
		}
		else if (strcmp(params[1], "PORT") == 0)
		{
			// set Host Port
			uint32_t port = atol(params[2]);
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (port > (uint32_t) 65535)
			{
				yprintf("Not Set: %s max value 65535\r\n", params[1]);
			}
			else
			{
				configStore->port = (uint16_t) port;
			}
		}
		else if (strcmp(params[1], "USERNAME") == 0)
		{
			// set user name
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (strlen(params[2]) > sizeof(configStore->username)-1)
			{
				yprintf("Not Set: %s max length %d\r\n", params[1], sizeof(configStore->username)-1);
			}
			else
			{
				memset(configStore->username, 0, sizeof(configStore->username));
				strcpy(configStore->username, params[2]);
			}
		}
		else if (strcmp(params[1], "PASSWORD") == 0)
		{
			// set password
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (strlen(params[2]) > sizeof(configStore->password)-1)
			{
				yprintf("Not Set: %s max length %d\r\n", params[1], sizeof(configStore->password)-1);
			}
			else
			{
				memset(configStore->password, 0, sizeof(configStore->password));
				strcpy(configStore->password, params[2]);
			}
		}
		else if (strcmp(params[1], "THING_TOKEN") == 0)
		{
			// set TOKEN - THING
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (strlen(params[2]) > sizeof(configStore->thing_token)-1)
			{
				yprintf("Not Set: %s max length %d\r\n", params[1], sizeof(configStore->thing_token)-1);
			}
			else
			{
				memset(configStore->thing_token, 0, sizeof(configStore->thing_token));
				strcpy(configStore->thing_token, params[2]);
			}
		}
		else if (strcmp(params[1], "ACCOUNT_TOKEN") == 0)
		{
			// set TOKEN - ACCOUNT
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (strlen(params[2]) > sizeof(configStore->account_token)-1)
			{
				yprintf("Not Set: %s max length %d\r\n", params[1], sizeof(configStore->account_token)-1);
			}
			else
			{
				memset(configStore->account_token, 0, sizeof(configStore->account_token));
				strcpy(configStore->account_token, params[2]);
			}
		}
		else if (strcmp(params[1], "REPORT_RATE") == 0)
		{
			uint32_t report_rate = atol(params[2]);
			yprintf("%s %s: %s\r\n", params[0], params[1], params[2]);
			if (report_rate > (uint32_t) 65535)
			{
				yprintf("Not Set: %s max value 65535\r\n", params[1]);
			}
			else
			{
				configStore->report_rate = (uint16_t) report_rate;
			}

		}
		else
		{
			yprintf("Unsupported set parameter: %s\r\n", params[1]);
		}
	}
	else if (strcmp(params[0], "COMMIT") == 0)
	{
		// Commit changes to flash
		yprintf("Commit changes to Flash\r\n");
		memset(nvBuffer, 0, sizeof(nvBuffer));
		memcpy(nvBuffer, configStore, sizeof(CONFIG_STRUCT));
		FLASH_If_Erase(USER_NV_STORE, USER_NV_STORE + USER_NV_STORE_SIZE);
		if (FLASH_If_Write(USER_NV_STORE, nvBuffer, sizeof(nvBuffer)/4) == FLASHIF_OK)
		{
			yprintf("Flash Write Successful\r\n");
		}
		else
		{
			yprintf("Flash write error\r\n");
		}
	}
	else if (strcmp(params[0], "DUMP") == 0)
	{
		if (paramCnt > 1)
		{
			uint32_t address = atol(params[1]);
			int count = 256;
			if (paramCnt > 2)
				count = atoi(params[2]);
			DumpMem("DUMP ", (char*) address, count);
		}
		else
		{
			DumpMem("NVSTORE", USER_NV_STORE, sizeof(CONFIG_STRUCT));

		}
	}
	else if (strcmp(params[0], "CONFIG") == 0)
	{
		ToUpper(params[1]);
		if (strcmp(params[1], "DEFAULT") == 0)
		{
			configStorePhysical = configDefault;
			DisplayConfig();
		}
		else if (strcmp(params[1], "RELOAD") == 0)
		{
			memcpy(&configStorePhysical, (CONFIG_STRUCT*)USER_NV_STORE, sizeof(CONFIG_STRUCT)) ;
			DisplayConfig();
		}
		else if (strcmp(params[1], "DUMP") == 0)
		{
			DumpMem("NVSTORE", USER_NV_STORE, sizeof(nvBuffer));
		}
		else if (strcmp(params[1], "ERASE") == 0)
		{
			FLASH_If_Erase(USER_NV_STORE, USER_NV_STORE + USER_NV_STORE_SIZE);
			yprintf("Erased nvStore\r\n");
		}
		else
		{
			yprintf("Config: %s is not supported\r\n", params[1]);
		}
	}
	else if (strcmp(params[0], "DISPLAY") == 0)
	{
		ToUpper(params[1]);
		if (strcmp(params[1], "CONFIG") == 0)
		{
			DisplayConfig();
		}
		else
		{
			yprintf("Displaying %s is not supported\r\n", params[1]);
		}
	}
	else if (strcmp(params[0], "DISCARD") == 0)
	{
		yprintf("Discarding changes\r\n");
	}
	else if (strcmp(params[0], "PRINT") == 0)
	{
		ToUpper(params[1]);
		if (strcmp(params[1], "OFF") == 0)
		{
			printOn = 0;
			yprintf("Turned off print logging\r\n");
		}
		else if (strcmp(params[1], "on") == 0)
		{
			yprintf("Turned on print logging\r\n");
			printOn = 1;
		}
		else
		{
			yprintf("Turned on print logging\r\n");
			printOn = 1;
		}
	}
	else if (strcmp(params[0], "RESET") == 0)
	{
		//
		int delay = 5000;
		if (paramCnt > 1)
			delay = atoi(params[1]);
		yprintf("Device Reset\r\n");
		osDelay(delay);
		NVIC_SystemReset();
	}
	else
	{
		yprintf("Command: %s not supported\r\n", params[0]);
	}
}

#define MAXINPUTSIZE 90
static void PrintThreadExec(const void *args)
{
	yprintf("System up and running: Version %d.%d.%d\r\n", MAINVERSION, MINORVERSION, POINTRELEASE);
	DisplayConfig();
	QueueUpNext(MAXINPUTSIZE);
	while (runflag)
	{
		osEvent event = osMessageGet(printMsgQid, osWaitForever); // 1.5 seconds
		if (event.status == osEventMessage)
		{
			char* msg = (char*) event.value.p;
			if (msg != NULL)
			{
				uint8_t cmd = (uint8_t) msg[0];
//				uint16_t size = *(uint16_t*) (msg+1);
				char *data = (char*) (msg+3);
				switch(cmd)
				{
					case CMD_PRINTF_DATA:
						HAL_UART_Transmit(&huart2, (uint8_t*) data, (uint16_t) strlen(data), 100);
					break;

					case CMD_ISR_DATA: // From user input on debug link
						{
							yprintf("\r\n");
							ProcessCommand(data);
//							HAL_UART_Transmit(&huart2, (uint8_t*) data, (uint16_t) strlen(data), 100);
							QueueUpNext(MAXINPUTSIZE);
						}
						break;
				}

				osFree(msg);
			}
		}
	}

	osThreadTerminate(printTaskHandle);
}

void PrintTaskDelete()
{
	runflag = 0;
}
#define PRINTLINEMAX 80
int xprintf(char* fmt, ...)
{
	uint16_t retsize = -1;
	if (printOn)
	{
		char* buffer = (char*) osMalloc(PRINTLINEMAX+3);
		uint16_t* size = (uint16_t*) (buffer+1);
		char* data = buffer+3;
		osStatus status;
		*size = -1;
		retsize = *size;
		if (buffer != NULL)
		{
			va_list args;
			va_start(args, fmt);

			buffer[0] = CMD_PRINTF_DATA; // Command 01 for display
			*size = vsnprintf(data, PRINTLINEMAX, fmt, args);
			retsize = *size;
			status = osMessagePut(printMsgQid, (uint32_t) buffer, 50);
			if (status != osOK)
			{
				*size = -1;
				retsize = *size;
				osFree(buffer); // Can't Post so free it!
			}

			va_end(args);
		}
	}
	return (int) retsize;
}

static int yprintf(char* fmt, ...)
{
	char* data = (char*) osMalloc(PRINTLINEMAX+3);
	uint16_t size = 0;
	osStatus status;
	size = 0;
	if (data != NULL)
	{
		va_list args;
		va_start(args, fmt);

		size = vsnprintf(data, PRINTLINEMAX, fmt, args);

		HAL_UART_Transmit(&huart2, (uint8_t*) data, (uint16_t) strlen(data), 100);

		osFree(data); // Can't Post so free it!

		va_end(args);
	}

	return (int) size;
}

/*******************************************************************
* DoneOnCR(handle) Function used to determine if the buffer meets  *
* complete criteria.  This is a CR for UART3                       *
*******************************************************************/
static uint16_t DoneOnCR(void* handle)
{
	uint16_t retval = 0;
	UART_HandleTypeDef* pUartH = (UART_HandleTypeDef*) handle;

	if (*(pUartH->pRxBuffPtr-1) == 0x0D) // Return after a carriage return
		retval = 1;

	return retval;
}

/* USART2 init function */
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
  NVIC_EnableIRQ(USART2_IRQn);
}

/*******************************************************************
* DebugInputPost()                                                *
* Used to post data to the task.                                   *
* Interface can be designed to the tasks requirements.             *
* Note: ISRs can post but the TmoMS must be zero for an ISR to use *
* it.                                                              *
*******************************************************************/
int DebugInputPost(uint32_t msg, uint32_t TmoMs)
{
	int retval = 0;
	osStatus status;
	// Post a command for the task with data
	// msg-1 will point to debug input command
	status = osMessagePut(printMsgQid, msg, TmoMs);
	if (status == osOK)
	{
		retval = 1;
	}

	return retval;
}

void PrintTaskCreate()
{
	  runflag = 1;

	  MX_USART2_UART_Init(115200);
	  printTaskHandle = osThreadCreate(osThread(PrintTask), NULL);
	  printMsgQid = osMessageCreate(&printMsgDefn, printTaskHandle);
}
