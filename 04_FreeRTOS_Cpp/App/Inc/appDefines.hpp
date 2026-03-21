#ifndef APP_DEFINES_HPP
#define APP_DEFINES_HPP

#pragma once
#include <stdint.h>

#ifdef UNIT_TEST
    #define PRIVATE_FOR_TEST public
#else
    #define PRIVATE_FOR_TEST private
#endif

#define STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE    0x11223344U
#define LOG_EVENT_SECTOR_VERSION                1u
#define LOGGER_MESSAGE_STR_LEN                  128u

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

    typedef enum
    {
        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE = 0,
        sAPPLOGGER_EVENT_CODE_LOG_INFO,
        sAPPLOGGER_EVENT_CODE_LOG_FAULT,
        sAPPLOGGER_EVENT_CODE_LOG_ERROR,
        sAPPLOGGER_EVENT_CODE_LOG_WARNING,
        
        sAPPLOGGER_EVENT_CODE_TOTAL
    } sAppLoggerEventCode_t;

    typedef enum {
        /* System Events */
        EVENT_ID_SYS_BOOT           = 0x1001U,
        EVENT_ID_SYS_FAULT          = 0x1002U,
        
        /* Sensor Events */
        EVENT_ID_SENSOR_READ_FAIL   = 0x2001U,
        EVENT_ID_SENSOR_DATA_POINT  = 0x2002U,
        EVENT_ID_T_SENSOR_DATA_POINT  = 0x2003U,
        EVENT_ID_H_SENSOR_DATA_POINT  = 0x2004U,
        
        /* Storage Events (Phase 3-M3) */
        EVENT_ID_QSPI_INIT_SUCCESS  = 0x3001U,
        EVENT_ID_QSPI_ERASE_BEGIN   = 0x3002U
    } eEventID_t;

    typedef struct __attribute__((packed))
    {
        uint32_t timestamp;   /* Result of xTaskGetTickCount() */
        eEventID_t eventID;     /* e.g., 0x1001 for Boot, 0x2001 for sensor Data */
        uint16_t taskID;      /* Which task is writing? */
        uint32_t payload[2];  /* Raw data (e.g., Temp, Error Code) */
    } sStorageEvent_t;

    typedef struct __attribute__((packed))
    {
        char pcMessage[LOGGER_MESSAGE_STR_LEN];
        sAppLoggerEventCode_t enumEventCode;
    } sAppLoggerMessage_t;

    typedef struct __attribute__((packed))
    {
        uint32_t magicSignature;
        uint16_t version;
        uint16_t maxEvents;
        uint32_t eraseCount;
        uint32_t reserved;
    } sLogSectorHeader_t;

    //#define LOG_SECTOR_SIZE         MX25R6435F_SECTOR_SIZE
    static constexpr uint32_t MAX_LOG_EVENTS = 255U;
    static constexpr uint32_t LOG_ENTRY_SIZE = sizeof(sStorageEvent_t);
    static constexpr uint32_t LOG_HEADER_SIZE = sizeof(sLogSectorHeader_t); //16 = Signature + Definition
    static constexpr uint32_t LOG_MESSAGE_SIZE = sizeof(sAppLoggerMessage_t);
    static constexpr uint32_t COMMAND_ENTRY_SIZE = sizeof(uint8_t);

    static constexpr uint32_t LOG_PARTITION_START = 0U;
    static constexpr uint32_t LOG_DATA_START = LOG_HEADER_SIZE;
    static constexpr uint32_t LOG_PARTITION_END = (LOG_DATA_START + (MAX_LOG_EVENTS * LOG_ENTRY_SIZE));
}

#endif //APP_DEFINES_HPP