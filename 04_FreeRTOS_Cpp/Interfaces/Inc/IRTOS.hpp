#ifndef I_RTOS_HPP
#define I_RTOS_HPP

#include <stdint.h>

namespace FreeRTOS_Cpp {
    class IRTOS {
    public:
        virtual ~IRTOS() = default;
        virtual void delay(uint32_t ms) = 0;
        virtual uint32_t getEventBits(void* handle) = 0;
        virtual void setEventBits(void* handle, uint32_t bits) = 0;
    };
}
#endif