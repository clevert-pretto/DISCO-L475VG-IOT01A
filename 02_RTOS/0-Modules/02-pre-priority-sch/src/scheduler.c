#include "scheduler.h"
#include "stm32l475xx.h"

#define STACK_MAGIC_NUMBER 0xDEADBEEF

TCB_t *currentTask = 0;
TCB_t *nextTask = 0;
TCB_t *taskHead = 0; // Head of the task list

void Task_Stack_Init(TCB_t *tcb, uint32_t *stack, uint32_t stackSize, 
                    void (*taskPtr)(void), uint32_t priority)
{
    
    /* 1. "Paint" the entire stack with a magic number */
    for (uint32_t i = 0; i < stackSize; i++) {
        stack[i] = STACK_MAGIC_NUMBER;
    }
    
    /* Point to the top of the stack (stack grows downwards) */
    uint32_t *psp = &stack[stackSize];

    /* 2. Initialize the Exception Frame as before */
    *(--psp) = 0x01000000;      /* xPSR: Set Thumb bit (Bit 24) */
    *(--psp) = (uint32_t)taskPtr | 0x1; /* Program Counter (PC): Start of Task, ensure LSB is 1 for Thumb mode */
    *(--psp) = 0xFFFFFFFD;      /* Link Register (LR): Return to Thread mode, use PSP */
    
    /* Dummy values for Hardware-saved registers: R12, R3, R2, R1, R0 */
    for(int i = 0; i < 5; i++) {
        *(--psp) = 0;
    }

    /* Dummy values for Software-saved registers: R11 to R4 */
    for(int i = 0; i < 8; i++) {
        *(--psp) = 0;
    }

    tcb->stackPtr = psp;        /* Save the resulting stack pointer into the TCB */
    tcb->priority = priority;   /* Save the priority */
    tcb->state = TASK_READY;    /* task state */
    tcb->sleepTicks = 0;        

    /* Add to global linked list */
    tcb->nextPtr = taskHead;
    taskHead = tcb;
}

void Scheduler_SelectNext(void) 
{
    TCB_t *temp = taskHead;
    TCB_t *highestPriorityTask = NULL;

    /* lop through all the tasks to find the highest priority task */
    while (temp != NULL) {
        if (temp->state == TASK_READY) {
            if (highestPriorityTask == NULL || 
                temp->priority < highestPriorityTask->priority) 
            {
                highestPriorityTask = temp;
            }
        }
        temp = temp->nextPtr;
    }

    /* CRITICAL: If no task is ready, something is wrong (Idle Task should be ready) */
    if (highestPriorityTask != NULL) {
        nextTask = highestPriorityTask;
        
        /* Only pend PendSV if we are actually changing tasks */
        if (nextTask != currentTask) {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
}

void Task_Sleep(uint32_t ticks) {
    __disable_irq();
    currentTask->sleepTicks = ticks;
    currentTask->state = TASK_BLOCKED;
    __enable_irq();
    
    Scheduler_SelectNext(); // Give up CPU immediately
}

/* Function to check how many bytes are still "pristine" */
uint32_t Get_Stack_Unused(uint32_t *stack, uint32_t stackSize) {
    uint32_t unused = 0;
    /* Start from the bottom (index 0) and count magic numbers */
    while (unused < stackSize && stack[unused] == STACK_MAGIC_NUMBER) {
        unused++;
    }
    return unused * sizeof(uint32_t); // Returns size in bytes
}