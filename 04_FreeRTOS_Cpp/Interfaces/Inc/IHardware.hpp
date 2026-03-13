#ifndef I_HARDWARE_HPP
#define I_HARDWARE_HPP

#include <stdint.h>

namespace FreeRTOS_Cpp {
    class IHardware {
    public:
        virtual ~IHardware() = default;
        virtual void toggleLed(uint16_t ledId) = 0;
    };
}
#endif