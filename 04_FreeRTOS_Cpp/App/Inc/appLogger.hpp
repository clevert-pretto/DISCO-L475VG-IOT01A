#ifndef APP_LOGGER_HPP
#define APP_LOGGER_HPP

#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "stm32l4xx_hal.h"


namespace FreeRTOS_Cpp
{
    #define LOGGER_MESSAGE_STR_LEN  128U
    #define STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE    0x11223344U
    #define LOG_EVENT_SECTOR_VERSION                1u

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
    #define MAX_LOG_EVENTS          255U
    #define LOG_ENTRY_SIZE          sizeof(sStorageEvent_t)
    #define LOG_HEADER_SIZE         sizeof(sLogSectorHeader_t) //16 = Signature + Definition
    #define LOG_PARTITION_START     0U
    #define LOG_DATA_START          LOG_HEADER_SIZE
    #define LOG_PARTITION_END       (LOG_DATA_START + (MAX_LOG_EVENTS * LOG_ENTRY_SIZE))

    #define LOG_MESSAGE_SIZE        sizeof(sAppLoggerMessage_t)

    #define COMMAND_ENTRY_SIZE      sizeof(uint8_t)

    // Forward declarations of structs to keep header clean
    // struct sStorageEvent_t;
    // struct sAppLoggerMessage_t;
    // struct sLogSectorHeader_t;

    class appLogger{

        public:
            //Constants
            static constexpr uint32_t DEFAULT_BAUDRATE = 115200;
            static constexpr size_t QUEUE_LEN = 20;
            static constexpr uint32_t MESSAGE_STR_LEN = 128;

            //Constructor
            appLogger(UART_HandleTypeDef* huart, QSPI_HandleTypeDef* hqspi, 
                EventGroupHandle_t sysEvents, EventGroupHandle_t wdgEvents);

            // Disable copy constructor and assignment operator for driver safety
            appLogger(const appLogger&) = delete;
            appLogger& operator=(const appLogger&) = delete;

            
            // Public API
            void init();
            uint8_t storageInit(void);
            static void logMessage(const char* pcMessage, sAppLoggerEventCode_t enumEventCode);
            static void logEvent(const sStorageEvent_t* event);
            
            /**
             * @brief FreeRTOS Task Trampolines
             */
            static void vAppLoggerTask(void* pvParameters);
            static void vCommandTask(void* pvParameters);
            static SemaphoreHandle_t _eraseCompleteMutex;

            // Global pointer for the ISR to access the instance
            static appLogger* instance;

            QueueHandle_t _commandQueue{nullptr};
            volatile uint8_t _rxChar{0};

            static bool storageInitStatus;

        private:
            // Hardware Handles
            UART_HandleTypeDef* _huart;
            QSPI_HandleTypeDef* _hqspi;

            // RTOS Resources (Now private members)
            // Static RTOS Control Blocks and Storage
            StaticQueue_t _printQueueBuffer;
            uint8_t _printQueueStorage[QUEUE_LEN * LOG_MESSAGE_SIZE];
            QueueHandle_t _printQueue{nullptr};

            StaticQueue_t _eventQueueBuffer;
            uint8_t _eventQueueStorage[QUEUE_LEN * LOG_ENTRY_SIZE];
            QueueHandle_t _eventQueue{nullptr};

            StaticQueue_t _commandQueueBuffer;
            uint8_t _commandQueueStorage[10U * COMMAND_ENTRY_SIZE];

            StaticSemaphore_t _uartMutexBuffer;
            SemaphoreHandle_t _uartMutex{nullptr};

            StaticSemaphore_t _qspiMutexBuffer;
            SemaphoreHandle_t _qspiMutex{nullptr};

            StaticSemaphore_t eraseCompleteMutexBuffer;
            
            /* tracking the head of the log */
            uint32_t _u32CurrentWriteAddress{0};
                    
            // For Watchdog event bits
            EventGroupHandle_t _wdgEvents;
            EventGroupHandle_t _sysEvents;

            // Internal Methods
            void uartInit(void);
            void scanWriteHead(void);
            void dumpLogs(void);
            void flushBufferToFlash(sStorageEvent_t* pBuffer, uint8_t count);
            void handleCommand(uint8_t rxChar);
            void sendCommandResponse(const char *pMsg);
            void eventSectorErase(void);
            void storageBulkErase(void);

            void checkStackUsage(void);
    };
}


#endif // APP_LOGGER_HPP