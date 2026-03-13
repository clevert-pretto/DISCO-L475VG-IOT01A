#ifndef APP_DEFINES_HPP
#define APP_DEFINES_HPP

#include <stdint.h>

namespace FreeRTOS_Cpp {
    // Event Bits (Pure Logic)
    static constexpr uint32_t EVENT_BIT_INIT_SUCCESS = (1U << 0);
    static constexpr uint32_t EVENT_BIT_INIT_FAILED  = (1U << 1);
    
    // Watchdog Bits
    static constexpr uint32_t WATCHDOG_BIT_HEARTBEAT = (1U << 0);

    // Hardware IDs (Abstraction IDs)
    static constexpr uint16_t HW_ID_STATUS_LED = 2; 
}

#endif