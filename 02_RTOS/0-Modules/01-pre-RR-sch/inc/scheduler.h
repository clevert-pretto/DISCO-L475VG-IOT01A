#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/* TCB must have stackPtr as the first member for easy Assembly access */
typedef struct TCB {
    uint32_t *stackPtr;      
    struct TCB *nextPtr;     
} TCB_t;

/* Function to initialize the stack of a task to look like an exception frame */
void Task_Stack_Init(TCB_t *tcb, uint32_t *stack, uint32_t stackSize, void (*taskPtr)(void));

uint32_t Get_Stack_Unused(uint32_t *stack, uint32_t stackSize);

/* Global pointer to the currently running task */
extern TCB_t *currentTask;

#endif