/*
 * PrintTask.h
 *
 *  Created on: Nov 20, 2016
 *      Author: Kevin Brown
 *
 *  A thread template for applications
 */

#ifndef PRINTTASK_H_
#define PRINTTASK_H_

void PrintTaskDelete(void);
int xprintf(char* fmt, ...);
void PrintTaskCreate(void);
int DebugInputPost(uint32_t msg, uint32_t TmoMs);
void LoadConfigFromFlash();

#endif /* PRINTTASK_H_ */
