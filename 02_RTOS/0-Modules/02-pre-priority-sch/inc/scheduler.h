#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>
#include <stddef.h>

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED
} TaskState_t;

typedef struct TCB {
    uint32_t *stackPtr;     /* Offset 0 */
    struct TCB *nextPtr;    /* Offset 4 */
    uint32_t priority;      /* Offset 8 Lower number = Higher priority */
    TaskState_t state;      /* Offset 12 Current status of the task */
    uint32_t sleepTicks;    /* Ticks remaining in sleep */
} TCB_t;

/* Function to initialize the stack of a task to look like an exception frame */
void Task_Stack_Init(TCB_t *tcb, uint32_t *stack, uint32_t stackSize, void (*taskPtr)(void), uint32_t priority);

/* Sleep for ticks duration*/
void Task_Sleep(uint32_t ticks);

/* Priority scheduling algorithm */
void Scheduler_SelectNext(void);

uint32_t Get_Stack_Unused(uint32_t *stack, uint32_t stackSize);



#endif //__SCHEDULER_H__