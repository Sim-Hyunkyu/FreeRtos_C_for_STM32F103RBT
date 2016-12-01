/*
 * Uart2Handler.h
 *
 *  Created on: Nov 20, 2016
 *      Author: Kevin Brown
 *
 *  A thread template for applications
 */

#ifndef UART2HANDLER_H_
#define UART2HANDLER_H_

// Public Thread Access
void Uart2HandlerCreate(void);

int Uart2HandlerPost(uint32_t msg, uint32_t TmoMs);

void DeleteUart2Handler(void);

#endif /* UART2HANDLER_H_ */
