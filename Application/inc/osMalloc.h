/*
 * osMalloc.h
 *
 *  Created on: Nov 22, 2016
 *      Author: Kevin
 */

#ifndef OSMALLOC_H_
#define OSMALLOC_H_

#include <stddef.h>

void osMallocInit(void);
void* osMalloc(int size);
void osFree(void* ptr);

size_t osMallocAvail();
size_t osMallocTotal();
size_t osMallocAllocated();

#endif /* OSMALLOC_H_ */
