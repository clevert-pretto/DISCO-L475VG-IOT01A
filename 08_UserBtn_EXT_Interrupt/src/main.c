#include "my_stm32_map.h"
#include "uart.h"

volatile uint8_t Button_Pressed = 0;
volatile uint32_t msTicks = 0;

void SysTick_Handler(void) { msTicks++; }

// Add this to your main.c or startup.c
void EXTI15_10_IRQHandler(void) {
    if (EXTI_PR1 & (1 << 13)) {
        EXTI_PR1 = (1 << 13); // W1C
        Button_Pressed = 1;
    }
}

void debug_button_init(void) {
    RCC_AHB2ENR |= (1UL << 2); // Port C
    RCC_APB2ENR |= (1 << 0);   // SYSCFG

    GPIOC_MODER &= ~(3UL << (13 * 2)); // Input
    
    // Route PC13 to EXTI13 (Index 4 is for lines 12-15)
    SYSCFG_EXTICR4 &= ~(0xF << 4);
    SYSCFG_EXTICR4 |=  (0x2 << 4); // Port C

    EXTI_FTSR1 |= (1 << 13); // Falling Edge (Press)
    EXTI_IMR1  |= (1 << 13); // Unmask

    NVIC_ISER1 |= (1 << (40 - 32)); // IRQ 40 is Bit 8 of ISER1
}

int main (void)
{
    uart_init(115200);
    debug_button_init();

    SYSTICK->LOAD = 3999; SYSTICK->VAL = 0; SYSTICK->CTRL = 0x07;

    RCC_AHB2ENR |= GPIOB_EN;
    GPIOB_MODER &= ~(3UL << (LED_PIN * 2)); GPIOB_MODER |= (1UL << (LED_PIN * 2));

    __asm volatile ("cpsie i" : : : "memory"); // Unmute interrupts

    uart_send_string("\r\n-- Interrupt Diagnostic Mode --\r\n");
    uart_send_string("1. Press BLUE BUTTON to test pipeline.\r\n");

    while(1)
    {
        if(Button_Pressed) {
            Button_Pressed = 0;
            uart_send_string("DEBUG: Button Interrupt OK! Pipeline is valid.\r\n");
            GPIOB_ODR ^= (1UL << LED_PIN);
        }
    }
}