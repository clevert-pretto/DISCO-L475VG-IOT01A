#include <stdint.h>

/* Base Addresses */
#define RCC_BASE      0x40021000UL
#define GPIOB_BASE    0x48000400UL

/* RCC Register for AHB2 Clock Enable (Offset 0x4C) */
#define RCC_AHB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x4C))

/* GPIOB Registers */
#define GPIOB_MODER   (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR     (*(volatile uint32_t *)(GPIOB_BASE + 0x14))

/* Bit positions */
#define GPIOB_EN      (1 << 1)     // Bit 1 in RCC_AHB2ENR enables GPIOB
#define LED_PIN       14           // The LED is on Pin 14

/* SysTick Registers (Cortex-M4 Private Peripheral Bus) */
#define SYSTICK_BASE  0xE000E010UL
typedef struct {
    volatile uint32_t CTRL;  /* Control and Status Register */
    volatile uint32_t LOAD;  /* Reload Value Register */
    volatile uint32_t VAL;   /* Current Value Register */
    volatile uint32_t CALIB; /* Calibration Register */
} SysTick_TypeDef;

#define SYSTICK ((SysTick_TypeDef *) SYSTICK_BASE)

#define CPU_FREQ    4000000UL  // 4MHz default
#define TICKS_PER_MS (CPU_FREQ / 1000)