#include "my_stm32_map.h"
#include "spi.h"

volatile uint8_t EXTI_IRQ_Check = 0;

void spi3_init (void)
{
    /* 1. Peripheral Clocks */
    RCC_APB1ENR1 |= (1 << 15);  //Enable SPI3 Clock
    RCC_APB2ENR |= (1 << 0);    //Enable SYSCFG Clock
    RCC_AHB2ENR |= (1UL << 0) | (1UL << 2) | (1UL << 4); // Port A, C, E
    RCC_APB1RSTR1 |= (1 << 15); //Reset SPI3
    RCC_APB1RSTR1 &= ~(1 << 15); //Clear SPI3

    // 2. Configure PC10, PC11, PC12 (SPI3 Pins) as AF6 (2UL)
    GPIOC_MODER &= ~((3UL << (SPI3_SCK_PIN * 2)) | (3UL << (SPI3_MISO_PIN * 2))
                                                 | (3UL << (SPI3_MOSI_PIN * 2)));

    GPIOC_MODER |= ((2UL << (SPI3_SCK_PIN * 2)) | (2UL << (SPI3_MISO_PIN * 2))
                                                 | (2UL << (SPI3_MOSI_PIN * 2)));
    
                                                 // 3. Set AF6 (SPI3) in AFRH for PC10, 11, 12
    GPIOC_AFRH &= ~((0xF << ((SPI3_SCK_PIN - 8) * 4)) | (0xF << ((SPI3_MISO_PIN- 8) * 4)) | 
                    (0xF << ((SPI3_MOSI_PIN- 8) * 4)));
    GPIOC_AFRH |= ((6UL << ((SPI3_SCK_PIN- 8) * 4)) | (6UL << ((SPI3_MISO_PIN- 8) * 4)) | 
                    (6UL << ((SPI3_MOSI_PIN- 8) * 4)));

    
    /* 3. Bluetooth Control Pins Configure PE0 (BT CS), PE1 (BT EXTI) and PA8 (BT RST) as Outputs */
    GPIOE_MODER &= ~((3UL << (SPI3_BLE_CSN_PIN * 2)));
    GPIOE_MODER |= ((1UL << (SPI3_BLE_CSN_PIN * 2)));
    GPIOE_ODR   |=  (1UL << SPI3_BLE_CSN_PIN); // Deselect (High)

    GPIOA_MODER &= ~(3UL << (BLE_RST * 2));
    GPIOA_MODER |=  (1UL << (BLE_RST * 2));
    GPIOA_ODR   &= ~(1UL << BLE_RST); // Keep in reset initially
    
    // 4. SPI Peripheral Config
    SPI3->CR1 = 0; // Clear CR1 completely to ensure a clean state
    SPI3->CR2 = 0; // Clear CR2 completely
    
    // CR1 Settings:
    // Bit 2: MSTR (Master selection)
    // Bit 8: SSI (Internal slave select) - Necessary for SSM
    // Bit 9: SSM (Software slave management)
    SPI3->CR1 |= ((1 << 2) | (1 << 8) | (1 << 9));

    // CR2 Settings:
    // Bits 8-11: DS (Data Size) set to 0111 (7) for 8-bit
    // Bit 12: FRXTH (FIFO reception threshold) - Trip RXNE on 8-bit
    SPI3->CR2 |= ((0x7 << 8) | (1 << 12)); 

    // 5. Enable SPI3
    SPI3->CR1 |= (1 << 6); // SPE (SPI Enable)
}

uint8_t spi3_transfer(uint8_t txByte)
{
    // 1. Wait until TXE (Transmit buffer empty) is set (Bit 1)
    while(!(SPI3->SR & (1 << 1)));

    // 2. Write the byte. 
    // Use a pointer cast to uint8_t to force an 8-bit write to the FIFO
    *((volatile uint8_t *) &SPI3->DR) = txByte;

    // 3. Wait until RXNE (Receive buffer not empty) is set (Bit 0)
    // Because SPI is full-duplex, the reply arrives as the command goes out
    while(!(SPI3->SR & (1 << 0)));

    // 4. Return the received byte via an 8-bit read
    return *((volatile uint8_t *)&SPI3->DR);
}