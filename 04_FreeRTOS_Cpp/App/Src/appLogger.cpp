// standard includes
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// hardware includes
#include "../main.hpp"
#include "stm32l4xx_hal_qspi.h"
#include "stm32l475e_iot01_qspi.h"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

// Application include
#include "main.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"
#include "xtoa.hpp"

appLogger* appLogger::instance = nullptr;
SemaphoreHandle_t appLogger::_eraseCompleteMutex = nullptr;
bool appLogger::storageInitStatus = false;

appLogger::appLogger(UART_HandleTypeDef* huart, QSPI_HandleTypeDef* hqspi, 
            EventGroupHandle_t sysEvents, EventGroupHandle_t wdgEvents) 
    : _huart(huart), _hqspi(hqspi), _printQueue(nullptr), _uartMutex(nullptr), 
    _wdgEvents(wdgEvents), _sysEvents(sysEvents)
{
    // Member initialization list is faster and safer than assignment in body
}

void appLogger::init(void)
{
    _printQueue = xQueueCreateStatic(QUEUE_LEN, LOG_MESSAGE_SIZE, _printQueueStorage, &_printQueueBuffer);
    
    _eraseCompleteMutex = xSemaphoreCreateBinaryStatic(&eraseCompleteMutexBuffer);

    _eventQueue = xQueueCreateStatic(QUEUE_LEN, sizeof(sStorageEvent_t), 
                                     _eventQueueStorage, &_eventQueueBuffer);
    
    _commandQueue = xQueueCreateStatic(10, sizeof(uint8_t), 
                                       _commandQueueStorage, &_commandQueueBuffer);
    
    _uartMutex = xSemaphoreCreateMutexStatic(&_uartMutexBuffer);
    _qspiMutex = xSemaphoreCreateMutexStatic(&_qspiMutexBuffer);

    appLogger::instance = this;
    // Initialize UART as Virtual COM Port
    uartInit();
    
    /* Manually enable the interrupt in NVIC (BSP usually skips this) */
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0); 
    HAL_NVIC_SetPriority(QUADSPI_IRQn, 6, 0); 
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_NVIC_EnableIRQ(QUADSPI_IRQn);
}

void appLogger:: uartInit(void)
{
    /* Configure the UART parameters BEFORE calling BSP_COM_Init */
    _huart->Instance = USART1; // Optional, BSP_COM_Init sets this too
    _huart->Init.BaudRate               = DEFAULT_BAUDRATE;
    _huart->Init.WordLength             = UART_WORDLENGTH_8B;
    _huart->Init.StopBits               = UART_STOPBITS_1;
    _huart->Init.Parity                 = UART_PARITY_NONE;
    _huart->Init.Mode                   = UART_MODE_TX_RX;
    _huart->Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    _huart->Init.OverSampling           = UART_OVERSAMPLING_16;
    _huart->Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    _huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    //HAL_UART_Init(_huart);
    BSP_COM_Init(COM1, _huart);
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
        xQueueSend(instance->_printQueue, &sLogMsg, 0);
    }
}

 void appLogger::logEvent(const sStorageEvent_t* event)
 {
    // Send to queue
    if ((instance != nullptr) && (instance->_eventQueue != nullptr)) {
        xQueueSend(instance->_eventQueue, event, 0);
    }
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
        while (xQueueReceive(self->_printQueue, &sLogMsg, 0) == pdPASS) {
            if (sLogMsg.enumEventCode == sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE) {
                if (xSemaphoreTake(self->_uartMutex, portMAX_DELAY) == pdTRUE) {
                    HAL_UART_Transmit(self->_huart, reinterpret_cast<uint8_t*>(sLogMsg.pcMessage), 
                                      static_cast<uint16_t>(strlen(sLogMsg.pcMessage)), 100);
                    xSemaphoreGive(self->_uartMutex);
                }
            }
        }

        // Handle binary events
        BaseType_t qStatus = xQueueReceive(self->_eventQueue, &sEvent_PageBuffer[eventCount], pdMS_TO_TICKS(100));
        if (qStatus == pdPASS) {
            eventCount++;
        }

        //Flush if buffer is full (16), OR if we have data and the queue timed out waiting for more */
        if ((eventCount >= 16) || ((qStatus == pdFAIL) && (eventCount > 0)))    
        {
            self->flushBufferToFlash(sEvent_PageBuffer, eventCount);
            eventCount = 0;
        }
        
        (void)xEventGroupSetBits(self->_wdgEvents, WATCHDOG_EVENT_BIT_TASK_APP_LOGGER);
    }
}

void appLogger::vCommandTask(void *pvParameters)
{
    uint8_t rxChar = 0;
    appLogger *self = static_cast<appLogger*>(pvParameters);

    /* Start the first interrupt-driven receive */
    HAL_UART_Receive_IT(self->_huart, const_cast<uint8_t*>(&self->_rxChar), 1);

    for (;;)
    {
        /* This task will SLEEP for 1000 sec */
        if (xQueueReceive(self->_commandQueue, &rxChar, 1000) == pdPASS)
        {
            self->handleCommand(rxChar);
        }
        
        (void)xEventGroupSetBits(self->_wdgEvents, WATCHDOG_EVENT_BIT_TASK_COMMAND);
    }
}

void appLogger::handleCommand(uint8_t rxChar)
{
    switch (rxChar)
    {
        case 'd':
            sendCommandResponse("Command: Triggering Flash Dump...\r\n");
            dumpLogs();
            break;

        case 'n':
            sendCommandResponse("Command: Triggering Event sector erase...\r\n");
            eventSectorErase();
            break;

        case 'p':
            sendCommandResponse("Command: Triggering Bulk Erase...\r\n");
            storageBulkErase();
            break;

        case 'h':
            sendCommandResponse("\r\n--- Command Menu ---\r\n"
                                 "d: Dump Logs\r\n"
                                 "p: Bulk Erase\r\n"
                                 "s: Stack Health\r\n"
                                 "n: Event log sector erase\r\n");
            break;

        case 's':
            sendCommandResponse("Command: Checking Stack Health...\r\n");
            checkStackUsage();
            break;

        default:
            sendCommandResponse("Command: Did you typed command in small letter?...\r\n");
            
            break;
    }
}

/* Internal function to dump Flash log (if found) on UART */
void appLogger::dumpLogs(void)
{
    sStorageEvent_t tempEvent;
    sLogSectorHeader_t sectorHeader;
    uint32_t readAddr = LOG_DATA_START; // Start after sector header data
    float fTemp;
    float fHum;
    static char outMsg[128];
    static char valBuf[16];

    if (xSemaphoreTake(_qspiMutex, portMAX_DELAY) == pdTRUE)
    {
        if(xSemaphoreTake(_uartMutex, portMAX_DELAY) == pdTRUE)
        {
            /* 2. READ AND PRINT SECTOR HEADER */
            if (BSP_QSPI_Read(reinterpret_cast<uint8_t*>(&sectorHeader), LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
            {
                (void)strcpy(outMsg, "--- FLASH LOG DUMP START ---\r\n--- SECTOR HEADER ---\r\n");
                (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)), 100);
                
                /* Print Erase Count */
                (void)strcpy(outMsg, "Signature: ");
                (void)memset(valBuf, 0, 16);
                xtoa::app_itoa(sectorHeader.magicSignature, valBuf, 15);
                (void)strcat(outMsg, valBuf);
                
                (void)strcat(outMsg, "\r\nVersion: ");
                (void)memset(valBuf, 0, 16);
                xtoa::app_itoa(sectorHeader.version, valBuf, 10);
                (void)strcat(outMsg, valBuf);
                
                (void)strcat(outMsg, "\r\nmax event: ");
                (void)memset(valBuf, 0, 16);
                xtoa::app_itoa(sectorHeader.maxEvents, valBuf, 10);
                (void)strcat(outMsg, valBuf);
                
                (void)strcat(outMsg, "\r\nErase Count: ");
                (void)memset(valBuf, 0, 16);
                xtoa::app_itoa(sectorHeader.eraseCount, valBuf, 12);
                (void)strcat(outMsg, valBuf);
                (void)strcat(outMsg, "\r\n---------------------\r\n");
                (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)), 100);
            }

            while (readAddr < _u32CurrentWriteAddress)
            {
                if (BSP_QSPI_Read(reinterpret_cast<uint8_t*>(&tempEvent), readAddr, LOG_ENTRY_SIZE) == QSPI_OK)
                {
                    /* Only process known sensor data points */
                    if (tempEvent.eventID == EVENT_ID_T_SENSOR_DATA_POINT)
                    {
                        /* Convert binary payload back to floats */
                        (void)memcpy(reinterpret_cast<void*>(&fTemp), reinterpret_cast<void*>(&tempEvent.payload[0]), sizeof(float));

                        /* Format output string: "Time: [tick] | T: [temp]" */
                        (void)memset(outMsg, 0, 128);
                        (void)memset(valBuf, 0, 16);
                        (void)strcpy(outMsg, "TS: ");
                        xtoa::app_itoa(tempEvent.timestamp, valBuf, 10);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, " | T: ");
                        xtoa::app_ftoa(fTemp, valBuf, 2);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, "\r\n");

                        /* Send directly to UART (bypassing queue for bulk dump) */
                        (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)), 100);
                    }
                    if (tempEvent.eventID == EVENT_ID_H_SENSOR_DATA_POINT)
                    {
                        (void)memcpy(&fHum, &tempEvent.payload[0], sizeof(float));
                        /* Format output string: "Time: [tick] | T: [temp] | H: [hum]" */
                        (void)strcpy(outMsg, "TS: ");
                        xtoa::app_itoa(tempEvent.timestamp, valBuf, 10);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, " | H: ");
                        xtoa::app_ftoa(fHum, valBuf, 2);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, "\r\n");

                        /* Send directly to UART (bypassing queue for bulk dump) */
                        (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(outMsg), static_cast<uint16_t>(strlen(outMsg)), 100);
                    }
                }
                readAddr += LOG_ENTRY_SIZE;
                
                /* Add a tiny delay so we don't overwhelm the UART or starve the Heartbeat */
                vTaskDelay(pdMS_TO_TICKS(1)); 
            }
            (void)xSemaphoreGive(_uartMutex);
        }
        (void)xSemaphoreGive(_qspiMutex);
    }

    logMessage("--- FLASH LOG DUMP END ---\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
}

/* Internal function to find where we left off */
void appLogger::scanWriteHead(void)
{
    sStorageEvent_t tempEvent;
    /* Lock the hardware for the initial scan */
    if (xSemaphoreTake(_qspiMutex, portMAX_DELAY) == pdTRUE)
    {
        bool headFound = false;
        uint32_t scanAddr = LOG_DATA_START; //first 16 bytes reserved for sector header data.
        /* Loop through flash in increments of our struct size */
        while (scanAddr < LOG_PARTITION_END)
        {
            //Read only 4 bytes of event ID size
            if (BSP_QSPI_Read(reinterpret_cast<uint8_t *>(&tempEvent), scanAddr, 4) == QSPI_OK)
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
            if (BSP_QSPI_Read(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
            {
                currentHeader.eraseCount++;
            }
            else
            {
                currentHeader.eraseCount = 1; // Fallback
            }

            /* Wipe the sector to make room for new logs */
            BSP_QSPI_Erase_Sector(LOG_PARTITION_START);
            while (BSP_QSPI_GetStatus() == QSPI_BUSY) { vTaskDelay(1); }

            /* 3. Re-write the header with the new count */
            currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
            currentHeader.version = LOG_EVENT_SECTOR_VERSION;
            currentHeader.maxEvents = MAX_LOG_EVENTS;
            BSP_QSPI_Write(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE);

            _u32CurrentWriteAddress = LOG_DATA_START;
            logMessage("Storage: Log Partition Full. Sector Erased & Wrapped.\r\n", 
                                    sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        else
        {
            char addrBuf[16];
            char msg[64] = {0};
            (void)strcpy(msg, "Storage: Resuming at addr: ");
            xtoa::app_itoa((int32_t)_u32CurrentWriteAddress, addrBuf, 10);
            (void)strncat(msg, addrBuf, sizeof(msg) - strlen(msg) - 1U);
            (void)strncat(msg, "\r\n", sizeof(msg) - strlen(msg) - 1U);
            logMessage(msg, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        (void)xSemaphoreGive(_qspiMutex);
    }
}

void appLogger::eventSectorErase(void)
{
    logMessage("Storage: Starting Event log sector Erase (Wait ~5s)...\r\n", 
                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

    if (xSemaphoreTake(_qspiMutex, portMAX_DELAY) == pdTRUE)
    {
        sLogSectorHeader_t currentHeader;
        /* Flash log sector is completely full, start at LOG_DATA_START */
        /* Read the old header to get the current erase count */
        //(void)memset(&currentHeader, 0, LOG_HEADER_SIZE);
        if (BSP_QSPI_Read(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
        {
            currentHeader.eraseCount++;
        }
        else
        {
            currentHeader.eraseCount = 1; // Fallback
        }

        /* Wipe the sector to make room for new logs */
        BSP_QSPI_Erase_Sector(LOG_PARTITION_START);
        while (BSP_QSPI_GetStatus() == QSPI_BUSY) 
        { 
            vTaskDelay(50);
            (void)HAL_IWDG_Refresh(&IWDG_handle);
            (void)xEventGroupSetBits(_wdgEvents, WATCHDOG_EVENT_BIT_TASK_COMMAND);
        }

        /* 3. Re-write the header with the new count */
        currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
        currentHeader.version = LOG_EVENT_SECTOR_VERSION;
        currentHeader.maxEvents = MAX_LOG_EVENTS;
        BSP_QSPI_Write(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE);

        _u32CurrentWriteAddress = LOG_DATA_START;
        (void)xSemaphoreGive(_qspiMutex);

        logMessage("Storage: Event log sector Erase Complete.\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
}

/**
  * @brief  Status Match callback for QSPI.
  * @param  hqspi: QSPI handle
  * @retval None
  */

/* cppcheck-suppress [constParameterPointer, misra-c2012-2.7] */
/* cppcheck-suppress [constParameterPointer, misra-c2012-8.4] */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
    /* Signal that the erase/write is physically complete on the silicon */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if (appLogger::_eraseCompleteMutex != NULL)
    {
        (void)xSemaphoreGiveFromISR(appLogger::_eraseCompleteMutex, &xHigherPriorityTaskWoken);
    }

    /* Force a context switch if the Command Task was waiting */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void appLogger::storageBulkErase(void)
{
    logMessage("Storage: Starting Full Chip Erase (Wait ~25s)...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

    if (xSemaphoreTake(_qspiMutex, portMAX_DELAY) == pdTRUE)
    {
        QSPI_CommandTypeDef s_command = {0};

        /* 1. Send WRITE ENABLE command */
        s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction       = 0x06; 
        s_command.AddressMode       = QSPI_ADDRESS_NONE;
        s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        s_command.DataMode          = QSPI_DATA_NONE;
        s_command.DummyCycles       = 0;
        s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
        s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
        s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

        if (HAL_QSPI_Command(_hqspi, &s_command, 100) == HAL_OK)
        {
            /* 2. Send CHIP ERASE command */
            s_command.Instruction = 0xC7; 
            if (HAL_QSPI_Command(_hqspi, &s_command, 100) == HAL_OK)
            {
                /* 3. Custom Non-Blocking RTOS Polling Loop */
                s_command.Instruction = 0x05; /* READ STATUS REGISTER */
                s_command.DataMode    = QSPI_DATA_1_LINE;
                s_command.NbData      = 1;
                uint8_t statusReg[1]  = {0xFF};

                do {
                    if (HAL_QSPI_Command(_hqspi, &s_command, 100) == HAL_OK) {
                        (void)HAL_QSPI_Receive(_hqspi, statusReg, 100);
                    }
                    /* Yield to RTOS and pet the watchdog to prevent the dead-board reset loop! */
                    vTaskDelay(pdMS_TO_TICKS(100));
                    (void)HAL_IWDG_Refresh(&IWDG_handle);
                    (void)xEventGroupSetBits(_wdgEvents, WATCHDOG_EVENT_BIT_TASK_COMMAND); 
                } while ((statusReg[0] & 0x01) == 0x01); /* While WIP (Write-In-Progress) bit is 1 */
            }
        }

        logMessage("Storage: Bulk Erase Complete.\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

        /* 4. Write the header exactly ONCE here */
        sLogSectorHeader_t currentHeader = {0};
        currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
        currentHeader.version = LOG_EVENT_SECTOR_VERSION;
        currentHeader.maxEvents = MAX_LOG_EVENTS;
        currentHeader.eraseCount = 1;
        
        if (BSP_QSPI_Write(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE) != QSPI_OK)
        {
             logMessage("CRITICAL: Failed to write Sector Header!\r\n", sAPPLOGGER_EVENT_CODE_LOG_ERROR);
        }
        
        _u32CurrentWriteAddress = LOG_DATA_START;
        (void)xSemaphoreGive(_qspiMutex);
    }
}

uint8_t appLogger::storageInit(void)
{
    QSPI_Info pInfo;
    char qspiMsg[LOGGER_MESSAGE_STR_LEN] = {0};
    char sizeBuf[16];
    uint8_t ret = (uint8_t)pdFAIL;
    sStorageEvent_t sEvent = {0};
    sLogSectorHeader_t header;

    //Initializes the QSPI interface
    if(BSP_QSPI_Init() == QSPI_OK)
    {
        /* Retrieve the Flash ID and configuration */
        if(BSP_QSPI_GetInfo(&pInfo) == QSPI_OK)
        {
            /* Format the flash memory capacity in MBytes*/
            uint32_t flashSizeMB = pInfo.FlashSize / (1024U * 1024U);
            /* Clear buffer and format string safely */
            qspiMsg[0] = '\0';
            (void)strncat(qspiMsg, "QSPI Init OK | Size: ", sizeof(qspiMsg) - 1U);
            
            /* Check if this is the first time the chip is being used */
            if(BSP_QSPI_Read(reinterpret_cast<uint8_t*>(&header), LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
            {
                if(header.magicSignature != STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE)
                {
                    /* Brand new chip or inconsistent data detected */
                    logMessage("Storage: First-time boot detected. Erasing chip...\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    
                    storageBulkErase(); /* This resets _u32CurrentWriteAddress to 0 */

                    /* Start logging events immediately after the sector header data */
                    _u32CurrentWriteAddress = LOG_HEADER_SIZE; 
                }
                else
                {
                    /* Normal Boot: Signature exists, just find where we left off */
                    logMessage("Storage: Magic Signature Found. Scanning flash...\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    scanWriteHead();
                }
            }

            /* Compose Structured Binary Event */
            sEvent.timestamp = xTaskGetTickCount();
            sEvent.eventID   = EVENT_ID_QSPI_INIT_SUCCESS;
            sEvent.taskID    = TASK_ID_SYS_MANAGER; // System Manager Task ID
            sEvent.payload[0] = pInfo.EraseSectorSize;      //Size of sectors for the erase operation
            sEvent.payload[1] = pInfo.ProgPageSize;         //Size of pages for the program operation

            logEvent(&sEvent);
            /* Quick integer to string conversion for the size */
            xtoa::app_itoa((int32_t)flashSizeMB, sizeBuf, 10);
            (void)strncat(qspiMsg, sizeBuf, sizeof(qspiMsg) - strlen(qspiMsg) - 1U);
            (void)strncat(qspiMsg, " MB\r\n", sizeof(qspiMsg) - strlen(qspiMsg) - 1U);

            logMessage(qspiMsg, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            ret = (uint8_t)pdPASS;
        }
    }
    return ret;
}

void appLogger::flushBufferToFlash(sStorageEvent_t *pBuffer, uint8_t eventCount)
{
    /* Calculate actual byte size to write */
    uint32_t writeSize = (uint32_t)eventCount * LOG_ENTRY_SIZE;
    sLogSectorHeader_t currentHeader;
    
    /* Hardware Protection */
    if (xSemaphoreTake(_qspiMutex, portMAX_DELAY) == pdTRUE)
    {
        /* Boundary Check: If we wrap, we must update the Erase Count*/
        if ((_u32CurrentWriteAddress + writeSize) >  LOG_PARTITION_END)
        {
            logMessage("Storage: Erasing Event logging Sector...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            
            //Read existing header to get the current count */
            if (BSP_QSPI_Read(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
            {
                currentHeader.eraseCount++; // Increment the life-cycle counter
            }
            else
            {
                /* Fallback if read fails (should not happen on healthy silicon) */
                currentHeader.eraseCount = 1; 
            }

            BSP_QSPI_Erase_Sector(LOG_PARTITION_START);
            
            /* Wait for Flash to finish erasing (Crucial for Reliability) */
            /* Wait for Flash to finish erasing (Crucial for Reliability) */
            while (BSP_QSPI_GetStatus() == QSPI_BUSY) 
            {
                vTaskDelay(pdMS_TO_TICKS(50)); 
                (void)HAL_IWDG_Refresh(&IWDG_handle);
                /* Note: This is called by vAppLoggerTask, so we feed its specific bit */
                (void)xEventGroupSetBits(_wdgEvents, WATCHDOG_EVENT_BIT_TASK_APP_LOGGER);
            }

            //Re-write the updated header at the start of the clean sector */
            currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
            currentHeader.version = LOG_EVENT_SECTOR_VERSION;
            currentHeader.maxEvents = MAX_LOG_EVENTS;
            // currentHeader.eraseCount is already incremented
            BSP_QSPI_Write(reinterpret_cast<uint8_t*>(&currentHeader), LOG_PARTITION_START, LOG_HEADER_SIZE);

            /* 4. Reset write head to the first data slot (Address 16) */
            _u32CurrentWriteAddress = LOG_DATA_START;
            
            logMessage("Storage: Sector Wrapped & Erase Count Updated\r\n", 
                                sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }

        /* 3. The Physical Write */
        /* Note: Most BSPs handle the 'Write Enable' internally inside the Write call */
        if (BSP_QSPI_Write(reinterpret_cast<uint8_t*>(pBuffer), _u32CurrentWriteAddress, writeSize) == QSPI_OK)
        {
            static sStorageEvent_t readBackBuffer[16]; // Temporary buffer for verification
            /* Verification Step: Read back exactly what we just wrote */
            if (BSP_QSPI_Read(reinterpret_cast<uint8_t*>(readBackBuffer), _u32CurrentWriteAddress, writeSize) == QSPI_OK)
            {
                /* Compare RAM vs. FLASH */
                if (memcmp(reinterpret_cast<const void*>(pBuffer), reinterpret_cast<const void*>(readBackBuffer), writeSize) == 0)
                {
                    //logMessage("Storage: Write Verified OK\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                }
                else
                {
                    logMessage("CRITICAL: Storage Verification Failed! Bit-flip detected.\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    // In a real SSD, you would mark this block as 'Bad' here.
                }
            }
        }
    }
    _u32CurrentWriteAddress += writeSize;
    /* 5. Release Resource */
    (void)xSemaphoreGive(_qspiMutex);
} 

// extern TaskHandle_t xAppLoggerTaskHandle;
// extern TaskHandle_t xAppCommandTaskHandle;
    
/* Internal function to print stack health for all tasks */
void appLogger::checkStackUsage(void)
{
    struct TaskMonitor {
        const char* name;
        TaskHandle_t handle;
    };

    const TaskMonitor monitoredTask[] = {
        {"Heartbeat", xHeartBeatTaskHandle},
        {"SysManager", xsystemManagerTaskHandle},
        {"SensorRead", xvSensorReadTaskHandle},
        {"Logger", xAppLoggerTaskHandle},
        {"Command", xAppLoggerTaskHandle}
    };

    char msg[128];
    char valBuf[16];

    if(xSemaphoreTake(_uartMutex, portMAX_DELAY) == pdTRUE)
    {
        (void)strcpy(msg,"\r\n--- STACK HIGH WATER MARKS (Words Remaining) ---\r\n");
        (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(msg), static_cast<uint16_t>(strlen(msg)), 10);

        for (const auto&  task: monitoredTask) {
            if (task.handle != nullptr) 
            {
                UBaseType_t watermark = uxTaskGetStackHighWaterMark(task.handle);
                
                // Use your new xtoa utility statically
                strncpy(msg, task.name, sizeof(msg)-1);
                strncat(msg, ": ", sizeof(msg)-strlen(msg)-1);
                xtoa::app_itoa(static_cast<uint32_t>(watermark), valBuf, 10);
                strncat(msg, valBuf, sizeof(msg)-strlen(msg)-1);
                strncat(msg, " words\r\n", sizeof(msg)-strlen(msg)-1);
                
                HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(msg), static_cast<uint16_t>(strlen(msg)), 100);
            }
        }

        (void)strcpy(msg,"-----------------------------------------------\r\n");
        (void)strcat(msg,"-------------- HEAP STATUS (Bytes) -----------\r\n");
        
        (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(msg), static_cast<uint16_t>(strlen(msg)), 100);
        (void)strcpy(msg, "Current Free Heap: ");
        xtoa::app_itoa(static_cast<uint32_t>(xPortGetFreeHeapSize()), valBuf, 10);
        (void)strcat(msg, valBuf);
        (void)strcat(msg, "\r\n---------------------------\r\n");

        (void)HAL_UART_Transmit(_huart, reinterpret_cast<uint8_t*>(msg), static_cast<uint16_t>(strlen(msg)), 100);
    
        (void)xSemaphoreGive(_uartMutex);
    }
}

/* cppcheck-suppress [constParameterPointer, misra-c2012-8.4] */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != NULL) 
    {
        if (huart->Instance == USART1 && appLogger::instance != nullptr)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            
            /* Send the received byte to the Command Task */
            (void)xQueueSendFromISR(appLogger::instance->_commandQueue, 
                                    const_cast<uint8_t*>(&appLogger::instance->_rxChar),
                          &xHigherPriorityTaskWoken);

            /* Re-enable the interrupt for the next character */
            (void)HAL_UART_Receive_IT(huart, const_cast<uint8_t*>(&appLogger::instance->_rxChar), 1u);

            /* Perform a context switch if a higher priority task was woken */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void appLogger::sendCommandResponse(const char *pMsg)
{
    if (xSemaphoreTake(_uartMutex, 100U) == pdTRUE)
    {
        (void)HAL_UART_Transmit(_huart, (const uint8_t*)pMsg, (uint16_t)strlen(pMsg), 100U);
        (void)xSemaphoreGive(_uartMutex);
    }
}