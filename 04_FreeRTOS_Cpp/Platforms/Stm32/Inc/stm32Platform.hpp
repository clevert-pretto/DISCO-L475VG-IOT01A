#ifndef STM32_PLATFORM_HPP
#define STM32_PLATFORM_HPP

#include "IRTOS.hpp"
#include "IHardware.hpp"

namespace FreeRTOS_Cpp {

    class Stm32Rtos : public IRTOS {
    public:
        void delay(uint32_t ms) override;
        uint32_t getEventBits(void* handle) override;
        void setEventBits(void* handle, uint32_t bits) override;
    };

    class Stm32Hardware : public IHardware {
    public:
        void toggleLed(uint16_t ledId) override;
    };

    extern Stm32Rtos realRtos;
    extern Stm32Hardware realHw;
}

#endif