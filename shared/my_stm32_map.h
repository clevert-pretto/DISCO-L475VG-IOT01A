#include <stdint.h>

#define CPU_FREQ    4000000UL  // 4MHz default
#define TICKS_PER_MS (CPU_FREQ / 1000)

/* Base Addresses */
#define RCC_BASE      0x40021000UL
#define GPIOA_BASE    0x48000000UL
#define GPIOB_BASE    0x48000400UL
#define USART1_BASE   0x40013800UL
#define I2C2_BASE     0x40005800UL
#define DMA1_BASE     0x40020000UL
#define DMA2_BASE     0x40020400UL

#define CPACR         (*(volatile uint32_t *)(0xE000ED88UL))

/* RCC Clock Control Register (Offset 0x00) */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))

/* RCC Register for AHB1 Clock Enable (Offset 0x48) */
#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x48))

/* RCC Register for AHB2 Clock Enable (Offset 0x4C) */
#define RCC_AHB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x4C))

#define RCC_APB1RSTR1   (*(volatile uint32_t *)(RCC_BASE + 0x38)) //Bit 17 is USART2 RST 1=RESET
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58)) //Bit 17 is USART2 EN  1=ENABLE. Bit 22 I2C2 EN
#define RCC_CCIPR       (*(volatile uint32_t *)(RCC_BASE + 0x88)) /*Bit 2, 3 for USART2SEL
                                                                        This bit is set and cleared by software to select the USART2 clock source.
                                                                        00: PCLK selected as USART2 clock
                                                                        01: System clock (SYSCLK) selected as USART2 clock
                                                                        10: HSI16 clock selected as USART2 clock
                                                                        11: LSE clock selected as USART2 clock*/
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08)) /* Bit 0:1 for PLL SRC*/
/* GPIOB Registers */
#define GPIOB_MODER   (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER  (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_ODR     (*(volatile uint32_t *)(GPIOB_BASE + 0x14))
#define GPIOB_AFRL    (*(volatile uint32_t *)(GPIOB_BASE + 0x20))
#define GPIOB_AFRH    (*(volatile uint32_t *)(GPIOB_BASE + 0x24))

/* Bit positions */
#define GPIOB_EN      (1 << 1)     // Bit 1 in RCC_AHB2ENR enables GPIOB
#define LED_PIN       14           // The LED is on Pin 14
#define VCP_TX_PIN    6            // USART1 VCP Pin PB6
#define VCP_RX_PIN    7            // USART1 VCP Pin PB7
#define I2C_2_SCL_PIN 10           // I2C2 SCL Pin PB10
#define I2C_2_SDA_PIN 11           // I2C2 SDA Pin PB11

#define NVIC_ISER1    (*(volatile uint32_t *) (0xE000E104UL)) //From armV7m architecture reference manual
#define NVIC_ISER0    (*(volatile uint32_t *) (0xE000E100)) //From armV7m architecture reference manual
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

} Usart_TypeDef;

#define USART1 ((Usart_TypeDef *) USART1_BASE)

typedef struct{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} I2c_TypeDef;

#define I2C2 ((I2c_TypeDef *) I2C2_BASE)

#define DMA1_CSELR (*(volatile uint32_t *)(DMA1_BASE + 0xA8))
#define DMA1_ISR (*(volatile uint32_t *)(DMA1_BASE + 0x00))
#define DMA1_IFCR (*(volatile uint32_t *)(DMA1_BASE + 0x04))
#define DMA1_CH4_CCR (*(volatile uint32_t *)(DMA1_BASE + 0x44))
#define DMA1_CH4_CNDTR (*(volatile uint32_t *)(DMA1_BASE + 0x48))
#define DMA1_CH4_CPAR (*(volatile uint32_t *)(DMA1_BASE + 0x4C))
#define DMA1_CH4_CMAR (*(volatile uint32_t *)(DMA1_BASE + 0x50))