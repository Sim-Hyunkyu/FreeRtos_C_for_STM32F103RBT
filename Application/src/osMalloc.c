/*
 * osMalloc.c
 *
 *  Created on: Nov 22, 2016
 *      Author: Kevin
 */

// Semaphore

#include <cmsis_os.h>
#include <malloc.h>
#include "osMalloc.h"

osSemaphoreDef (memMutex);                                     // Mutex name definition
osSemaphoreId mem_mutex_id;

void osMallocInit()
{
	mem_mutex_id = osSemaphoreCreate  (osSemaphore (memMutex), 1);
	  if (mem_mutex_id != NULL)  {
	    // Mutex object created

	  }
}

void* osMalloc(int size)
{
	void* retval = (void*) 0;
     osStatus status  = osSemaphoreWait (mem_mutex_id, 100);
    if (status == osOK)
    {
    	retval = malloc(size);
    	status = osSemaphoreRelease(mem_mutex_id);
    }

	return retval;
}

void osFree(void* ptr)
{
    osStatus status  = osSemaphoreWait (mem_mutex_id, osWaitForever);
    if (status == osOK)
    {
    	free(ptr);
    	status = osSemaphoreRelease(mem_mutex_id);
    }
}

size_t osMallocAvail()
{
	struct mallinfo allocInfo = mallinfo();
	return allocInfo.fordblks;
}

size_t osMallocTotal()
{
	struct mallinfo allocInfo = mallinfo();
	return allocInfo.arena;
}

size_t osMallocAllocated()
{
	struct mallinfo allocInfo = mallinfo();
	return allocInfo.uordblks;
}
