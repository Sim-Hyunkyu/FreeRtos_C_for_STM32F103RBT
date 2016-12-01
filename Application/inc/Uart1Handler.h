/*
 * Uart2Handler.h
 *
 *  Created on: Nov 20, 2016
 *      Author: Kevin Brown
 *
 *  A thread template for applications
 */

#ifndef UART1HANDLER_H_
#define UART1HANDLER_H_

// Public Thread Access
void Uart1HandlerCreate(void);

int Uart1HandlerPost(uint32_t msg, uint32_t TmoMs);

void DeleteUart1Handler(void);

#endif /* UART2HANDLER_H_ */
