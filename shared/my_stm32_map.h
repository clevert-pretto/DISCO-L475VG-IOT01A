#include <stdint.h>

#define CPU_FREQ    4000000UL  // 4MHz default
#define TICKS_PER_MS (CPU_FREQ / 1000)

/* Base Addresses */
#define RCC_BASE      0x40021000UL
#define GPIOA_BASE    0x48000000UL
#define GPIOB_BASE    0x48000400UL
#define USART1_BASE   0x40013800UL

/* RCC Clock Control Register (Offset 0x00) */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))

/* RCC Register for AHB2 Clock Enable (Offset 0x4C) */
#define RCC_AHB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x4C))

#define RCC_APB1RSTR1   (*(volatile uint32_t *)(RCC_BASE + 0x38)) //Bit 17 is USART2 RST 1=RESET
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58)) //Bit 17 is USART2 EN  1=ENABLE
#define RCC_CCIPR       (*(volatile uint32_t *)(RCC_BASE + 0x88)) /*Bit 2, 3 for USART2SEL
                                                                        This bit is set and cleared by software to select the USART2 clock source.
                                                                        00: PCLK selected as USART2 clock
                                                                        01: System clock (SYSCLK) selected as USART2 clock
                                                                        10: HSI16 clock selected as USART2 clock
                                                                        11: LSE clock selected as USART2 clock*/

/* GPIOB Registers */
#define GPIOB_MODER   (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR     (*(volatile uint32_t *)(GPIOB_BASE + 0x14))
#define GPIOB_AFRL    (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_AFRH    (*(volatile uint32_t *)(GPIOB_BASE + 0x24))

/* Bit positions */
#define GPIOB_EN      (1 << 1)     // Bit 1 in RCC_AHB2ENR enables GPIOB
#define LED_PIN       14           // The LED is on Pin 14
#define VCP_TX_PIN    6            // USART1 VCP Pin PB6
#define VCP_RX_PIN    7            // USART1 VCP Pin PB7

#define NVIC_ISER1    (*(volatile uint32_t *) (0xE000E104UL))

/* SysTick Registers (Cortex-M4 Private Peripheral Bus) */
#define SYSTICK_BASE  0xE000E010UL
typedef struct {
    volatile uint32_t CTRL;  /* Control and Status Register */
    volatile uint32_t LOAD;  /* Reload Value Register */
    volatile uint32_t VAL;   /* Current Value Register */
    volatile uint32_t CALIB; /* Calibration Register */
} SysTick_TypeDef;

#define SYSTICK ((SysTick_TypeDef *) SYSTICK_BASE)

typedef struct{
    volatile uint32_t CTRL1;  /*control register 1*/
    volatile uint32_t CTRL2;  /*control register 2*/
    volatile uint32_t CTRL3;  /*control register 3*/
    volatile uint32_t BRR;    /*Baud rate register */
    volatile uint32_t GTPR;  /*guard time and prescaler register*/
    volatile uint32_t RTOR;  /*receiver timeout register*/
    volatile uint32_t RQR;  /*request register*/
    volatile uint32_t ISR;  /*interrupt and status register*/
    volatile uint32_t ICR;  /*interrupt flag clear register*/
    volatile uint32_t RDR;  /*receive data register*/
    volatile uint32_t TDR;  /*transmit data register*/

} Usart2_TypeDef;

#define USART1 ((Usart2_TypeDef *) USART1_BASE)
#define USART2 ((Usart2_TypeDef *) USART2_BASE)