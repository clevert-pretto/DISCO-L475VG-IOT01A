// Application include
#include "appLogger.hpp"
#include "sysManager.hpp"
#include "xtoa.hpp"
#include <algorithm> // For std::find_if
#include <iterator>  // For std::begin, std::end

namespace FreeRTOS_Cpp
{
    appLogger* appLogger::instance = nullptr;
    bool appLogger::storageInitStatus = false;
    void* appLogger::_eraseCompleteMutex = nullptr;

   appLogger::appLogger(IRTOS* rtos, IHardware* hw, appSensorRead* sensorTask, void* sysEvents, void* wdgEvents, void* wdgHardwareHandle) 
        : _rtos(rtos), _hw(hw), _sensorTask(sensorTask), _sysEvents(sysEvents), _wdgEvents(wdgEvents), _watchdogHardware(wdgHardwareHandle)
    {
        // Member initialization list is faster and safer than assignment in body
    }

    void appLogger::init(void* printQueue, void* eventQueue, void* commandQueue, 
                         void* uartMutex, void* qspiMutex, void* eraseCompleteMutex)
    {
        // Simply store the injected handles
        _printQueue = printQueue;
        _eventQueue = eventQueue;
        _commandQueue = commandQueue;
        _uartMutex = uartMutex;
        _qspiMutex = qspiMutex;
        _eraseCompleteMutex = eraseCompleteMutex;
        appLogger::instance = this;
    }

    void appLogger::logMessage ( const char *pcMessage, 
                                sAppLoggerEventCode_t enumEventCodes)
    {
        
        sAppLoggerMessage_t sLogMsg;
        sLogMsg.enumEventCode = enumEventCodes;
        
        (void)strncpy(sLogMsg.pcMessage, pcMessage, sizeof(sLogMsg.pcMessage) - 1U);
        sLogMsg.pcMessage[sizeof(sLogMsg.pcMessage) - 1U] = '\0';

        // Send to queue
        
        if ((instance != nullptr) && (instance->_printQueue != nullptr)) {
            instance->_rtos->queueSend(instance->_printQueue, &sLogMsg, 0);
        }
    }

    void appLogger::logEvent(const sStorageEvent_t* event)
    {
        // Send to queue
        if ((instance != nullptr) && (instance->_eventQueue != nullptr)) {
            instance->_rtos->queueSend(instance->_eventQueue, event, 0);
        }
    }

    void appLogger::notifyCommandReceivedFromISR(uint8_t rxChar)
    {
        bool higherPriorityTaskWoken = false;
        
        // Safety check before using abstract RTOS call
        if ((_rtos != nullptr) && (_commandQueue != nullptr)) {
            _rtos->queueSendFromISR(_commandQueue, &rxChar, &higherPriorityTaskWoken);
        }
        
        // Note: FreeRTOS requires a context switch if a higher priority task was woken.
        // Since we are decoupling, the actual portYIELD_FROM_ISR is handled in 
        // HAL_QSPI_StatusMatchCallback or the UART ISR directly in main.cpp.
    }

    //TODO: MOdify it to prepend the EVENT CODE related string before the actual message
    void appLogger::vAppLoggerTask(void *pvParameters)
    {
        // Casting back to the instance
        appLogger* self = static_cast<appLogger*>(pvParameters);
        sAppLoggerMessage_t sLogMsg;
        sStorageEvent_t sEvent_PageBuffer[16];
        uint8_t eventCount = 0;

        for (;;)
        {
            // Drain print queue
            while (self->_rtos->queueReceive(self->_printQueue, &sLogMsg, 0)) 
            {
                if (sLogMsg.enumEventCode == sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE) 
                {
                    if (self->_rtos->takeMutex(self->_uartMutex, 0xFFFFFFFF)) 
                    {
                        self->_hw->printLog(reinterpret_cast<uint8_t*>(sLogMsg.pcMessage), 
                                        static_cast<uint16_t>(strlen(sLogMsg.pcMessage)));
                        self->_rtos->giveMutex(self->_uartMutex);
                    }
                }
            }

            // Handle binary events
            bool qStatus = self->_rtos->queueReceive(self->_eventQueue, &sEvent_PageBuffer[eventCount], 100);
            if (qStatus)
            {
                eventCount++;
            }

            //Flush if buffer is full (16), OR if we have data and the queue timed out waiting for more */
            if ((eventCount >= 16) || (!qStatus && (eventCount > 0)))    
            {
                self->flushBufferToFlash(sEvent_PageBuffer, eventCount);
                eventCount = 0;
            }
            
            self->_rtos->setEventBits(self->_wdgEvents, WATCHDOG_BIT_LOGGER);
        }
    }

    void appLogger::vCommandTask(void *pvParameters)
    {
        uint8_t rxChar = 0;
        char commandLine[32] = {0};
        uint8_t cmdIndex = 0;
        appLogger *self = static_cast<appLogger*>(pvParameters);

        /* Start the first interrupt-driven receive */
        self->_hw->startCommandReceiveIT((&self->_rxChar));

        for (;;)
        {
            /* This task will SLEEP for 1000 sec */
            if (self->_rtos->queueReceive(self->_commandQueue, &rxChar, 1000))
            {
                if(rxChar == '\n' || rxChar == '\r')
                {
                    commandLine[cmdIndex] = '\0';
                    if(cmdIndex > 0)
                    {
                        self->handleCommand(commandLine);
                    }
                    cmdIndex = 0;
                    self->sendCommandResponse(">"); // Display '>' on the terminal
                }
                else if (cmdIndex < sizeof(commandLine) - 1)
                {
                    commandLine[cmdIndex++] = rxChar;
                    #if DEBUGGING
                        self->_hw->printLog(&rxChar, 1); // Display the received character on the terminal
                    #endif
                }
            }
            self->_rtos->setEventBits(self->_wdgEvents, WATCHDOG_BIT_COMMAND);
        }
    }

/**
 * @brief Handles commands from the command task
 *
 * @param cmd Command string received from the command task
 *
 * @details Handles commands from the command task. Supports the following commands:
 *  - ping: Responds with "pong\r\n"
 *  - help: Prints the command help menu
 *  - dump_logs: Dumps the event logs
 *  - event_sector_erase: Erases the event log sector
 *  - bulk_erase: Performs a bulk erase of the storage
 *  - stack_health: Checks the stack health
 *  - get_temp: Gets the current temperature
 *  - get_humidity: Gets the current humidity
 *
 */
    void appLogger::handleCommand(const char* cmd)
    {
        // 1. Define a structure for our command table
        struct CommandEntry {
            const char* name;
            void (appLogger::*handler)();
        };


        // 2. The Command Table (Internal to function or class member)
        static const CommandEntry commandTable[] = {
            {"ping",                 &appLogger::cmdPing},
            {"help",                 &appLogger::cmdHelp},
            {"dump_logs",            &appLogger::dumpLogs},
            {"event_sector_erase",   &appLogger::eventSectorErase},
            {"bulk_erase",           &appLogger::storageBulkErase},
            {"stack_health",         &appLogger::checkStackUsage},
            {"get_temp",             &appLogger::cmdGetTemp},
            {"get_humidity",         &appLogger::cmdGetHumidity},
            {"enable_temp_log",      &appLogger::cmdEnableTempLog},
            {"disable_temp_log",     &appLogger::cmdDisableTempLog},
            {"enable_humidity_log",  &appLogger::cmdEnableHumLog},
            {"disable_humidity_log", &appLogger::cmdDisableHumLog}
        };

        // 3. STL Dispatcher logic using std::find_if
        auto it = std::find_if(std::begin(commandTable), 
                               std::end(commandTable),
                               [cmd](const CommandEntry& entry) 
            {
                return strncmp(cmd, entry.name, strlen(entry.name)) == 0;
            });

        // 4. Check if the command was found
        if (it != std::end(commandTable)) 
        {
            (this->*(it->handler))(); // Call the member function
        } 
        else 
        {
            sendCommandResponse("\r\nCommand: Invalid command\r\n");
        }
    }

    void appLogger::cmdPing() 
    {
        sendCommandResponse("\r\npong\r\n");
    }

    void appLogger::cmdGetTemp() {
        char tempBuf[16]; // Increased for safety
        sendCommandResponse("\r\nCurrent Temperature: ");
        
        // FIX: Use sizeof(tempBuf) instead of hardcoded '6' to prevent safety return
        xtoa::app_ftoa(_sensorTask->getCurrentTemp(), tempBuf, (uint32_t)sizeof(tempBuf));
        
        sendCommandResponse(tempBuf);
        sendCommandResponse(" C\r\n"); // Segmented printing saves ~40 bytes of stack
    }

    void appLogger::cmdGetHumidity() {
        char humBuf[16];
        sendCommandResponse("\r\nCurrent Humidity: ");
        xtoa::app_ftoa(_sensorTask->getCurrentHumidity(), humBuf, (uint32_t)sizeof(humBuf));
        sendCommandResponse(humBuf);
        sendCommandResponse(" %\r\n");
    }

    void appLogger::cmdHelp() {
        sendCommandResponse("\r\n--- Command Help Menu ---\r\n"
                            "dump_logs:             Dump Logs\r\n"
                            "bulk_erase:            Bulk Erase\r\n"
                            "stack_health:          Stack Health\r\n"
                            "get_temp:              Get Temperature\r\n"
                            "get_humidity:          Get Humidity\r\n"
                            "enable_temp_log:       Enable Temperature Logging\r\n"
                            "disable_temp_log:      Disable Temperature Logging\r\n"
                            "enable_humidity_log:   Enable Humidity Logging\r\n"
                            "disable_humidity_log:  Disable Humidity Logging\r\n"
                            "event_sector_erase:    Event log sector erase\r\n");
    }

    void appLogger::cmdEnableTempLog() {
        _sensorTask->enableTemperatureLogging();
        sendCommandResponse("\r\nCommand: Enabled Temperature Logging\r\n");
    }

    void appLogger::cmdDisableTempLog() {
        _sensorTask->disableTemperatureLogging();
        sendCommandResponse("\r\nCommand: Disabled Temperature Logging\r\n");
    }

    void appLogger::cmdEnableHumLog() {
        _sensorTask->enableHumidityLogging();
        sendCommandResponse("\r\nCommand: Enabled Humidity Logging\r\n");
    }

    void appLogger::cmdDisableHumLog() {
        _sensorTask->disableHumidityLogging();
        sendCommandResponse("\r\nCommand: Disabled Humidity Logging\r\n");
    }

    /* Internal function to dump Flash log (if found) on UART */
    void appLogger::dumpLogs(void)
    {
        sStorageEvent_t tempEvent;
        sLogSectorHeader_t sectorHeader;
        
        static char outMsg[128] = {0};
        static char valBuf[16] = {0};

        if (_rtos->takeMutex(_qspiMutex, 0xFFFFFFFF))
        {
            if(_rtos->takeMutex(_uartMutex, 0xFFFFFFFF))
            {
                /* 2. READ AND PRINT SECTOR HEADER */
                if (_hw->storageRead(reinterpret_cast<uint8_t*>(&sectorHeader), LOG_PARTITION_START, LOG_HEADER_SIZE))
                {
                    (void)strcpy(outMsg, "--- FLASH LOG DUMP START ---\r\n--- SECTOR HEADER ---\r\nSignature: ");
                    xtoa::app_itoa(sectorHeader.magicSignature, valBuf, 15);
                    (void)strcat(outMsg, valBuf);
                    (void)strcat(outMsg, "\r\nVersion: ");
                    xtoa::app_itoa(sectorHeader.version, valBuf, 10);
                    (void)strcat(outMsg, valBuf);
                    (void)strcat(outMsg, "\r\nmax event: ");
                    xtoa::app_itoa(sectorHeader.maxEvents, valBuf, 10);
                    (void)strcat(outMsg, valBuf);
                    (void)strcat(outMsg, "\r\nErase Count: ");
                    xtoa::app_itoa(sectorHeader.eraseCount, valBuf, 12);
                    (void)strcat(outMsg, valBuf);
                    (void)strcat(outMsg, "\r\n---------------------\r\n");
                    _hw->printLog(reinterpret_cast<const uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)));
                }

                uint32_t readAddr = LOG_DATA_START; // Start after sector header data
                while (readAddr < _u32CurrentWriteAddress)
                {
                    if (_hw->storageRead(reinterpret_cast<uint8_t*>(&tempEvent), readAddr, LOG_ENTRY_SIZE))
                    {
                        /* Only process known sensor data points */
                        if (tempEvent.eventID == EVENT_ID_T_SENSOR_DATA_POINT || tempEvent.eventID == EVENT_ID_H_SENSOR_DATA_POINT)
                        {                      
                            float sensorValue = 0.0f; // Use a generic name for clarity
                            (void)memcpy(&sensorValue, &tempEvent.payload[0], sizeof(float));
                            (void)memset(outMsg, 0, 128);
                            (void)strcpy(outMsg, "Event log: ");

                            xtoa::app_itoa(tempEvent.timestamp, valBuf, 10);
                            (void)strcat(outMsg, valBuf);

                            (void)strcat(outMsg, (tempEvent.eventID == EVENT_ID_T_SENSOR_DATA_POINT) ? " | T: " : " | H: ");
                            xtoa::app_ftoa(sensorValue, valBuf, 6);
                            (void)strcat(outMsg, valBuf);
                            (void)strcat(outMsg, "\r\n");
                            _hw->printLog(reinterpret_cast<const uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)));
                        }
                    }
                    readAddr += LOG_ENTRY_SIZE;
                    
                    /* Add a tiny delay so we don't overwhelm the UART or starve the Heartbeat */
                    _rtos->delay(1);
                }
                (void)strcpy(outMsg, "--- FLASH LOG DUMP END ---\r\n");
                _hw->printLog(reinterpret_cast<const uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)));
                _rtos->delay(1);
                _rtos->giveMutex(_uartMutex);
            }
            _rtos->giveMutex(_qspiMutex);
        }
    }

    /* Internal function to find where we left off */
    void appLogger::scanWriteHead(void)
    {
        sStorageEvent_t tempEvent;
        /* Lock the hardware for the initial scan */
        if (_rtos->takeMutex(_qspiMutex, 0xFFFFFFFFu))
        {
            bool headFound = false;
            uint32_t scanAddr = LOG_DATA_START; //first 16 bytes reserved for sector header data.
            /* Loop through flash in increments of our struct size */
            while (scanAddr < LOG_PARTITION_END)
            {
                //Read only 4 bytes of event ID size
                if (_hw->storageRead(reinterpret_cast<uint8_t *>(&tempEvent), scanAddr, 4))
                {
                    /* 0xFFFFFFFF means this 'slot' is empty */
                    if (tempEvent.timestamp == 0xFFFFFFFFU)
                    {
                        _u32CurrentWriteAddress = scanAddr;
                        headFound = true;
                        break;
                    }
                }
                scanAddr += LOG_ENTRY_SIZE;
            }

            if (!headFound)
            {
                sLogSectorHeader_t currentHeader;
                /* Flash log sector is completely full, start at LOG_DATA_START */
                /* Read the old header to get the current erase count */
                //(void)memset(&currentHeader, 0, LOG_HEADER_SIZE);
                if (_hw->storageRead(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE))
                {
                    currentHeader.eraseCount++;
                }
                else
                {
                    currentHeader.eraseCount = 1; // Fallback
                }

                /* Wipe the sector to make room for new logs */
                _hw->storageEraseSector(LOG_PARTITION_START);
                while (_hw->storageIsBusy()) { _rtos->delay(1); }

                /* 3. Re-write the header with the new count */
                currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
                currentHeader.version = LOG_EVENT_SECTOR_VERSION;
                currentHeader.maxEvents = MAX_LOG_EVENTS;
                _hw->storageWrite(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE);

                _u32CurrentWriteAddress = LOG_DATA_START;
                logMessage("Storage: Log Partition Full. Sector Erased & Wrapped.\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            }
            else
            {
                char addrBuf[16];
                char msg[64] = "Storage: Resuming at addr: ";
                xtoa::app_itoa((int32_t)_u32CurrentWriteAddress, addrBuf, 10);
                (void)strncat(msg, addrBuf, sizeof(msg) - strlen(msg) - 1U);
                (void)strncat(msg, "\r\n", sizeof(msg) - strlen(msg) - 1U);
                logMessage(msg, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            }
            _rtos->giveMutex(_qspiMutex);
        }
    }

    void appLogger::eventSectorErase(void)
    {
        logMessage("Storage: Starting Event log sector Erase (Wait ~5s)...\r\n", 
                            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

        if (_rtos->takeMutex(_qspiMutex, 0xFFFFFFFF))
        {
            sLogSectorHeader_t currentHeader;
            /* Flash log sector is completely full, start at LOG_DATA_START */
            /* Read the old header to get the current erase count */
            //(void)memset(&currentHeader, 0, LOG_HEADER_SIZE);
            if (_hw->storageRead(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE))
            {
                currentHeader.eraseCount++;
            }
            else
            {
                currentHeader.eraseCount = 1; // Fallback
            }

            /* Wipe the sector to make room for new logs */
            _hw->storageEraseSector(LOG_PARTITION_START);
            while (_hw->storageIsBusy()) 
            { 
                _rtos->delay(50);
                _hw->watchdog_refresh(_watchdogHardware);
                _rtos->setEventBits(_wdgEvents, WATCHDOG_BIT_COMMAND);
            }

            /* 3. Re-write the header with the new count */
            currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
            currentHeader.version = LOG_EVENT_SECTOR_VERSION;
            currentHeader.maxEvents = MAX_LOG_EVENTS;
            _hw->storageWrite(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE);

            _u32CurrentWriteAddress = LOG_DATA_START;
            _rtos->giveMutex(_qspiMutex);

            sendCommandResponse("Storage: Event log sector Erase Complete.\r\n");
        }
    } 

    void appLogger::storageBulkErase(void)
    {
        logMessage("Storage: Starting Full Chip Erase (Wait ~25s)...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

        if (_rtos->takeMutex(_qspiMutex, 0xFFFFFFFF))
        {
            if (_hw->storageBulkErase()) {
                while (_hw->storageIsBusy()) {
                    _rtos->delay(100);
                    _hw->watchdog_refresh(_watchdogHardware);
                    _rtos->setEventBits(_wdgEvents, WATCHDOG_BIT_COMMAND); 
                }
            }
            sendCommandResponse("Storage: Bulk Erase Complete.\r\n");
            sLogSectorHeader_t currentHeader = {0};
            currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
            currentHeader.version = LOG_EVENT_SECTOR_VERSION;
            currentHeader.maxEvents = MAX_LOG_EVENTS;
            currentHeader.eraseCount = 1;
            
            if (!_hw->storageWrite(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE)) {
                logMessage("CRITICAL: Failed to write Sector Header!\r\n", sAPPLOGGER_EVENT_CODE_LOG_ERROR);
            }
            
            _u32CurrentWriteAddress = LOG_DATA_START;
            _rtos->giveMutex(_qspiMutex);
        }
    }

    uint8_t appLogger::storageInit(void)
    {
        uint32_t flashSize, eraseSize, progSize;
        char qspiMsg[LOGGER_MESSAGE_STR_LEN] = "QSPI Init OK | Size: ";
        char sizeBuf[16];
        uint8_t ret = 0; // pdFAIL equivalent
        sStorageEvent_t sEvent = {0};
        sLogSectorHeader_t header;

        if(_hw->storageInit(&flashSize, &eraseSize, &progSize))
        {
            uint32_t flashSizeMB = flashSize / (1024U * 1024U);
            
            if(_hw->storageRead(reinterpret_cast<uint8_t*>(&header), LOG_PARTITION_START, LOG_HEADER_SIZE))
            {
                if(header.magicSignature != STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE) {
                    logMessage("Storage: First-time boot detected. Erasing chip...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    storageBulkErase(); 
                    _u32CurrentWriteAddress = LOG_HEADER_SIZE; 
                } else {
                    logMessage("Storage: Magic Signature Found. Scanning flash...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    scanWriteHead();
                }
            }

            sEvent.timestamp = _rtos->getTickCount();
            sEvent.eventID   = EVENT_ID_QSPI_INIT_SUCCESS;
            sEvent.taskID    = TASK_ID_SYS_MANAGER; 
            sEvent.payload[0] = eraseSize;      
            sEvent.payload[1] = progSize;         

            logEvent(&sEvent);
            xtoa::app_itoa((int32_t)flashSizeMB, sizeBuf, 10);
            (void)strncat(qspiMsg, sizeBuf, sizeof(qspiMsg) - strlen(qspiMsg) - 1U);
            (void)strncat(qspiMsg, " MB\r\n", sizeof(qspiMsg) - strlen(qspiMsg) - 1U);

            logMessage(qspiMsg, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            ret = 1; // pdPASS equivalent
        }
        return ret;
    }

    void appLogger::flushBufferToFlash(sStorageEvent_t *pBuffer, uint8_t eventCount)
    {
        uint32_t writeSize = (uint32_t)eventCount * LOG_ENTRY_SIZE;
        sLogSectorHeader_t currentHeader;
        
        if (_rtos->takeMutex(_qspiMutex, 0xFFFFFFFF))
        {
            if ((_u32CurrentWriteAddress + writeSize) >  LOG_PARTITION_END)
            {
                logMessage("Storage: Erasing Event logging Sector...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                if (_hw->storageRead(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE)) {
                    currentHeader.eraseCount++; 
                } else {
                    currentHeader.eraseCount = 1; 
                }

                _hw->storageEraseSector(LOG_PARTITION_START);
                
                while (_hw->storageIsBusy()) 
                {
                    _rtos->delay(50); 
                    _hw->watchdog_refresh(_watchdogHardware);
                    _rtos->setEventBits(_wdgEvents, WATCHDOG_BIT_LOGGER);
                }

                currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
                currentHeader.version = LOG_EVENT_SECTOR_VERSION;
                currentHeader.maxEvents = MAX_LOG_EVENTS;
                _hw->storageWrite(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE);

                _u32CurrentWriteAddress = LOG_DATA_START;
                logMessage("Storage: Sector Wrapped & Erase Count Updated\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            }

            if (_hw->storageWrite(reinterpret_cast<uint8_t*>(pBuffer), _u32CurrentWriteAddress, writeSize))
            {
                static sStorageEvent_t readBackBuffer[16]; 
                if (_hw->storageRead(reinterpret_cast<uint8_t*>(readBackBuffer), _u32CurrentWriteAddress, writeSize))
                {
                    if (memcmp(reinterpret_cast<const void*>(pBuffer), reinterpret_cast<const void*>(readBackBuffer), writeSize) != 0) {
                        logMessage("CRITICAL: Storage Verification Failed! Bit-flip detected.\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    }
                }
            }
        }
        _u32CurrentWriteAddress += writeSize;
        _rtos->giveMutex(_qspiMutex);
    } 

    void appLogger::checkStackUsage(void)
    {
        char valBuf[16];
        const char* taskName = nullptr;
        void* taskHandle = nullptr;
        sendCommandResponse("\r\n--- Task Stack High Water Marks (Words Remaining) ---\r\n");

        uint32_t count = _rtos->getRegisteredTaskCount();
        for (uint32_t i = 0; i < count; i++)
        {
            if (_rtos->getRegisteredTaskInfo(i, &taskName, &taskHandle)) 
            {
                // 1. Print Task Name
                sendCommandResponse(taskName);
                sendCommandResponse(": ");

                // 2. Get and convert Water Mark
                // Note: uxTaskGetStackHighWaterMark returns the minimum free stack space 
                // seen since the task started. Lower = Closer to overflow.
                uint32_t stackRemaining = _rtos->getStackHighWaterMark(taskHandle);
                xtoa::app_itoa(static_cast<int32_t>(stackRemaining), valBuf, 10);
                
                // 3. Print Value
                sendCommandResponse(valBuf);
                sendCommandResponse(" words\r\n");
            } 
            else 
            {
                sendCommandResponse(taskName);
                sendCommandResponse(": NOT_STARTED\r\n");
            }
        }
        sendCommandResponse("----------------------------------------------------\r\n");
    }

    void appLogger::sendCommandResponse(const char *pMsg)
    {
        if (_rtos->takeMutex(_uartMutex, 100))
        {
            _hw->printLog(reinterpret_cast<const uint8_t*>(pMsg), (uint16_t)strlen(pMsg));
            _rtos->giveMutex(_uartMutex);
        }
    }
}