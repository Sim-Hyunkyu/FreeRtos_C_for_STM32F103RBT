/*
 * main.h
 *
 *  Created on: Nov 19, 2016
 *      Author: Kevin
 */

#ifndef MAIN_H_
#define MAIN_H_

#define MAINVERSION     0
#define MINORVERSION    0
#define POINTRELEASE    1

#include <stm32f1xx_hal.h>

typedef enum
{
	CMD_NOT_SPECIFIED,
	CMD_ISR_DATA,
	CMD_PRINTF_DATA,
	CMD_CONTROL_INFO,

	// add more commands before this line
	CMD_TOTAL_CMD_CNT
} COMMANDS;

typedef struct
{
	//TCP connection settings structure
	char host[81];
	uint16_t port;
	//Cellular connection settings structure
	char apn[21];
	//Mqtt client settings structure
	char username [21];
	char password [21];
	char thing_token [51];
	char account_token [51];
	//Application settings structure
	uint16_t report_rate; //in seconds
} CONFIG_STRUCT;

extern CONFIG_STRUCT* configStore;
extern CONFIG_STRUCT configStorePhysical;

#endif /* MAIN_H_ */
