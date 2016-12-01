/*
 * UartHandler.h
 *
 *  Created on: Nov 20, 2016
 *      Author: Kevin Brown
 *
 *  A thread template for applications
 */

#ifndef UART3HANDLER_H_
#define UART3HANDLER_H_

// Public Thread Access
void Uart3HandlerCreate(void);

int Uart3ThreadPost(uint32_t msg, uint32_t TmoMs);

void DeleteUart3Handler(void);

#endif /* UART3HANDLER_H_ */
