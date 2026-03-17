#ifndef APP_DEFINES_HPP
#define APP_DEFINES_HPP

#include <stdint.h>

#ifdef UNIT_TEST
    #define PRIVATE_FOR_TEST public
#else
    #define PRIVATE_FOR_TEST private
#endif

namespace FreeRTOS_Cpp {
    // Event Bits (Pure Logic)
    static constexpr uint32_t EVENT_BIT_INIT_SUCCESS = (1U << 0u);
    static constexpr uint32_t EVENT_BIT_INIT_FAILED  = (1U << 1u);
    static constexpr uint32_t EVENT_BIT_FAULT_DETECTED  = (1U << 2u);
    
    // Watchdog Bits
    static constexpr uint32_t WATCHDOG_BIT_LOGGER       = (1U << 0u);
    static constexpr uint32_t WATCHDOG_BIT_HEARTBEAT    = (1U << 1u);
    static constexpr uint32_t WATCHDOG_BIT_SENSOR_READ  = (1U << 2u);
    static constexpr uint32_t WATCHDOG_BIT_COMMAND      = (1U << 3u);

    static constexpr uint32_t WATCHDOG_MANDATORY_BITMASK = (WATCHDOG_BIT_LOGGER | \
                                                  WATCHDOG_BIT_HEARTBEAT | \
                                                  WATCHDOG_BIT_SENSOR_READ | \
                                                  WATCHDOG_BIT_COMMAND);
    // Hardware IDs (Abstraction IDs)
    static constexpr uint8_t HW_ID_STATUS_LED = 2u;

    //Task priorities
    static constexpr uint8_t  TASK_PRIORITY_HEARTBEAT_TASK    = 0u;
    static constexpr uint8_t TASK_PRIORITY_SYS_MANAGER_TASK   = 3u;
    static constexpr uint8_t  TASK_PRIORITY_SENSOR_READ_TASK  = 1u;
    static constexpr uint8_t  TASK_PRIORITY_APPLOGGER_TASK    = 2u;
    static constexpr uint8_t  TASK_PRIORITY_COMMAND_TASK      = 2u;

    //Task stack sizes
    static constexpr uint32_t TASK_MINIMAL_STACK_SIZE = 128u;
    static constexpr uint32_t TASK_STACK_SIZE_HEARTBEAT_TASK    = (TASK_MINIMAL_STACK_SIZE * 2u);
    static constexpr uint32_t TASK_STACK_SIZE_SYS_MANAGER_TASK  = (TASK_MINIMAL_STACK_SIZE * 2u);
    static constexpr uint32_t TASK_STACK_SIZE_SENSOR_READ_TASK  = (TASK_MINIMAL_STACK_SIZE * 4u);
    static constexpr uint32_t TASK_STACK_SIZE_APPLOGGER_TASK    = (TASK_MINIMAL_STACK_SIZE * 5u);
    static constexpr uint32_t TASK_STACK_SIZE_COMMAND_TASK      = (TASK_MINIMAL_STACK_SIZE * 4u);

    //Watchdog timeout
    static constexpr uint32_t IWDG_TIMEOUT_ms = 5000U;

    //Task IDs
    static constexpr uint8_t  TASK_ID_SYS_MANAGER = 1U;
    static constexpr uint8_t  TASK_ID_HEART_BEAT = 2U;
    static constexpr uint8_t  TASK_ID_SENSOR_READ = 3U;
    static constexpr uint8_t  TASK_ID_APP_LOGGER = 4U;
}

#endif