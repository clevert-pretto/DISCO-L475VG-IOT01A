#include "stm32l475xx.h"
#include "scheduler.h"

/* Define Stacks and TCBs */
uint32_t stackA[128] __attribute__((aligned(8)));
uint32_t stackB[128] __attribute__((aligned(8)));
uint32_t stackIdle[64] __attribute__((aligned(8)));
TCB_t tcbA, tcbB, tcbIdle;

extern TCB_t *currentTask;
extern TCB_t *nextTask;
extern TCB_t *taskHead;

void TaskIdle(void) {
    while(1) {
        /* In a real world scenario, we would put the CPU to sleep here to save power */
        __WFI(); 
    }
}

void Clock_Config_16MHz(void) {
    // 1. Enable HSI16
    RCC->CR |= RCC_CR_HSION;

    // 2. Wait until HSI16 is ready
    while (!(RCC->CR & RCC_CR_HSIRDY));

    // 3. Set Flash Wait States (Latency)
    // At 16MHz and 3.3V, 0 wait states is technically okay, 
    // but 1 wait state (1WS) is safer for stability.
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_0WS; 

    // 4. Select HSI16 as system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;      // Clear bits
    RCC->CFGR |= RCC_CFGR_SW_HSI;   // Select HSI

    // 5. Wait until HSI is used as system clock
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
    
    // 6. Optional: Disable MSI to save power
    RCC->CR &= ~RCC_CR_MSION;
}

void SysTick_Init(uint32_t ticks) {
    SysTick->LOAD  = (uint32_t)(ticks - 1UL);                         /* Set reload register */
    NVIC_SetPriority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* Set lowest priority */
    SysTick->VAL   = 0UL;                                             /* Load the SysTick Counter Value */
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk   |
                     SysTick_CTRL_ENABLE_Msk;                         /* Enable SysTick */
    
    
    NVIC_SetPriority(SysTick_IRQn, 14); 
}

void SysTick_Handler(void) 
{
    TCB_t *temp = taskHead;
    /* Update sleep timers */
    while (temp != NULL) {
        if (temp->state == TASK_BLOCKED) {
            if (temp->sleepTicks > 0) {
                temp->sleepTicks--;
            } else {
                temp->state = TASK_READY;
            }
        }
        temp = temp->nextPtr;
    }
    Scheduler_SelectNext();
}

void SystemInit(void) {
    /* Set VTOR to the start of FLASH if not already done by hardware */
    SCB->VTOR = FLASH_BASE;
}

void GPIO_Init(void) {
    /* 1. Enable Clock for Port A and Port B */
    RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN);

    /* 2. Configure PA5 (LED1) as Output */
    GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);
    GPIOA->MODER |= (1 << GPIO_MODER_MODE5_Pos);

    /* 3. Configure PB14 (LED2) as Output */
    GPIOB->MODER &= ~(GPIO_MODER_MODE14_Msk);
    GPIOB->MODER |= (1 << GPIO_MODER_MODE14_Pos);
}

volatile uint32_t freeBytes;
void TaskA(void) {
    while(1) {
        GPIOA->BSRR = GPIO_BSRR_BS5; // LED ON
        Task_Sleep(500);             // Yields CPU, non-blocking
        GPIOA->BSRR = GPIO_BSRR_BR5; // LED OFF
        Task_Sleep(500);

        /* Monitor stack usage */
        freeBytes = Get_Stack_Unused(stackA, 128);
        if (freeBytes < 32) {
            /* Handle Stack Overflow Danger! */
        }

    }
}

void TaskB(void) {
    while(1) {
        GPIOB->BSRR = GPIO_BSRR_BS14; // LED2 ON
        Task_Sleep(1000);
        GPIOB->BSRR = GPIO_BSRR_BR14; // LED2 OFF
        Task_Sleep(1000);
    }
}

int main(void) {

    Clock_Config_16MHz();

    /* Explicitly disable FPU to ensure standard 8-word hardware stack frame */
    /* Access the Coprocessor Access Control Register (CPACR) */
    SCB->CPACR &= ~((3UL << 20) | (3UL << 22));

    /* 1. Hardware Init */
    /* Configure LED Pins */
    GPIO_Init();

    /* 2. Scheduler Init taskA priority = 0, taskB priority = 1 */
    Task_Stack_Init(&tcbA, stackA, 128, TaskA, 0);
    Task_Stack_Init(&tcbB, stackB, 128, TaskB, 1);
    Task_Stack_Init(&tcbIdle, stackIdle, 64, TaskIdle, 255); // Lowest Priority

    /* Ensure these are set before sysTickInit() to allow pendSV a valid source*/
    currentTask = &tcbA;
    nextTask = &tcbA;

    /* 3. SysTick & Priority Configuration */
    /* HSI Clock is 16MHz by default; set 1ms tick */
    SysTick_Init(16000);

    /* 4. Launch First Task */
    /* Set PendSV to the lowest possible priority (15 for STM32L4) */
    NVIC_SetPriority(PendSV_IRQn, 15);
    __set_PSP((uint32_t)tcbA.stackPtr); /* Set PSP to Task A stack */
    __set_CONTROL(0x02);               /* Switch to PSP, Thread Mode */
    __ISB();                           /* Instruction Synchronization Barrier */
    __enable_irq();                    /* Enable Global Interrupt */
    
    
    TaskA(); /* Start Task A */
    //while(1); /* No effect of while here, as CPU never going to come here again */
}

