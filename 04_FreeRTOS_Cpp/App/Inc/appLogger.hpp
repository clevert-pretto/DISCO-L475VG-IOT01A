#ifndef APP_LOGGER_HPP
#define APP_LOGGER_HPP


#include <stdint.h>
#include <string.h>
#include "IRTOS.hpp"     
#include "IHardware.hpp"
#include "appDefines.hpp"

#define STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE    0x11223344U
#define LOG_EVENT_SECTOR_VERSION                1u
#define LOGGER_MESSAGE_STR_LEN                  128u

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

namespace FreeRTOS_Cpp
{

    // Forward declarations of structs to keep header clean
    // struct sStorageEvent_t;
    // struct sAppLoggerMessage_t;
    // struct sLogSectorHeader_t;

    class appLogger{

        public:
            //Constants
            static constexpr uint32_t DEFAULT_BAUDRATE = 115200u;
            static constexpr size_t QUEUE_LEN = 20u;
            static constexpr uint32_t MESSAGE_STR_LEN = 128u;

            //Constructor
            appLogger(IRTOS* rtos, IHardware* hw, 
                      void* sysEvents, void* wdgEvents, void* wdgHardwareHandle);

            // Disable copy constructor and assignment operator for driver safety
            appLogger(const appLogger&) = delete;
            appLogger& operator=(const appLogger&) = delete;

            void init(void* printQueue, void* eventQueue, void* commandQueue, 
                         void* uartMutex, void* qspiMutex, void* eraseCompleteMutex);
            uint8_t storageInit(void);

            // Static methods for easy calling across the application
            static void logMessage(const char* pcMessage, sAppLoggerEventCode_t enumEventCode);
            static void logEvent(const sStorageEvent_t* event);
            static void vAppLoggerTask(void* pvParameters);
            static void vCommandTask(void* pvParameters);

            // Public method so the platform ISR can pass data to the queue
            void notifyCommandReceivedFromISR(uint8_t rxChar);
            volatile uint8_t* getRxBuffer() { return &_rxChar; }
            
            // Global pointer for the ISR to access the instance
            static appLogger* instance;
            static bool storageInitStatus;
            static void* _eraseCompleteMutex;

        PRIVATE_FOR_TEST:
            
            IRTOS* _rtos;               // Abstracted RTOS
            IHardware* _hw;             // Abstracted Hardware

            // RTOS Resources (Now private members)
            // Static RTOS Control Blocks and Storage
            void* _printQueue{nullptr};
            void* _eventQueue{nullptr};
            void* _commandQueue{nullptr};
            void* _uartMutex{nullptr};
            void* _qspiMutex{nullptr};
            
            /* tracking the head of the log */
            uint32_t _u32CurrentWriteAddress{0};
                    
            // For Watchdog event bits
            void* _sysEvents;
            void* _wdgEvents;
            void* _watchdogHardware;    // Raw hardware handle for petting
            volatile uint8_t _rxChar{0};

            // Internal Methods
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