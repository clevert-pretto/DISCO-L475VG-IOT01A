.syntax unified
.cpu cortex-m4
.fpu softvfp    /* Keep FPU disabled for now to simplify context switching */
.thumb

.global PendSV_Handler
.type PendSV_Handler, %function

PendSV_Handler:
    /* 1. Save Current Task Context */
    MRS R0, PSP             /* Get current Process Stack Pointer */
    STMDB R0!, {R4-R11}     /* Push R4-R11 manually to Task Stack */
    
    LDR R1, =currentTask    /* Load address of currentTask pointer */
    LDR R1, [R1]            /* Get current TCB address */
    STR R0, [R1]            /* Save current SP (R0) into current TCB->stackPtr */

    /* 2. Switch to Next Task */
    LDR R1, =currentTask
    LDR R1, [R1]            /* Get current TCB address */
    LDR R1, [R1, #4]        /* Load currentTask->nextPtr (offset 4) */
    LDR R2, =currentTask
    STR R1, [R2]            /* Update currentTask = currentTask->nextPtr */

    /* 3. Restore Next Task Context */
    LDR R0, [R1]            /* Load new task's stackPtr into R0 */
    LDMIA R0!, {R4-R11}     /* Pop R4-R11 from new Task Stack */
    MSR PSP, R0             /* Update PSP with new Task Stack Pointer */

    BX LR                   /* Return from exception, hardware pops R0-R3, PC, etc. */