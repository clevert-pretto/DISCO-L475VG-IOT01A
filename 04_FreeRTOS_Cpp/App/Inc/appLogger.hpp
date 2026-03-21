#ifndef APP_LOGGER_HPP
#define APP_LOGGER_HPP


#include <stdint.h>
#include <string.h>
#include "appDefines.hpp"
#include "IRTOS.hpp"     
#include "IHardware.hpp"
#include "appSensorRead.hpp"

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
            appLogger(IRTOS* rtos, IHardware* hw, appSensorRead* sensorTask, 
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
            
            // Internal RTOS Control Blocks
            appSensorRead* _sensorTask; 

            // For Watchdog event bits
            void* _sysEvents;
            void* _wdgEvents;
            void* _watchdogHardware;    // Raw hardware handle for petting
            volatile uint8_t _rxChar{0};

            // Internal Methods
            void scanWriteHead(void);
            void dumpLogs(void);
            void flushBufferToFlash(sStorageEvent_t* pBuffer, uint8_t count);
            void handleCommand(const char* cmd);
            void sendCommandResponse(const char *pMsg);
            void eventSectorErase(void);
            void storageBulkErase(void);
            void checkStackUsage(void);
            void cmdPing(void);
            void cmdHelp(void);
            void cmdDumpLogs(void);
            void cmdStackHealth(void);
            void cmdGetTemp(void);
            void cmdGetHumidity(void);
            void cmdEnableTempLog(void);
            void cmdDisableTempLog(void);
            void cmdEnableHumLog(void);
            void cmdDisableHumLog(void);
    };
}


#endif // APP_LOGGER_HPP