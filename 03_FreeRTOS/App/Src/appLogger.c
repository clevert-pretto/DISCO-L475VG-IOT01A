// standard includes
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// hardware includes
#include "../main.h"
#include "stm32l4xx_hal_qspi.h"
#include "stm32l475e_iot01_qspi.h"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

// Application include
#include "main.h"
#include "appLogger.h"
#include "sysManager.h"
#include "xtoa.h"

//extern UART_HandleTypeDef discoveryUART1;
static QueueHandle_t xPrintQueue;
static QueueHandle_t xCommandQueue;
static QueueHandle_t xEventQueue;
static SemaphoreHandle_t xQSPIMutex = NULL;
static SemaphoreHandle_t xUART1Mutex = NULL;

/* Global variable tracking the head of the log */
static uint32_t u32CurrentWriteAddress = 0U;
/* Physical memory allocation for the UART RX buffer */
static volatile uint8_t g_rxChar = 0;

/* Internal function to dump Flash log (if found) on UART */
static void appLogger_DumpLogs(void)
{
    sStorageEvent_t tempEvent;
    sLogSectorHeader_t sectorHeader;
    uint32_t readAddr = LOG_DATA_START; // Start after sector header data
    float fTemp;
    float fHum;
    static char outMsg[128];
    static char valBuf[16];

    if (xSemaphoreTake(xQSPIMutex, portMAX_DELAY) == pdTRUE)
    {
        if(xSemaphoreTake(xUART1Mutex, portMAX_DELAY) == pdTRUE)
        {
            /* 2. READ AND PRINT SECTOR HEADER */
            if (BSP_QSPI_Read((uint8_t *)&sectorHeader, LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
            {
                (void)strcpy(outMsg, "--- FLASH LOG DUMP START ---\r\n--- SECTOR HEADER ---\r\n");
                (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)outMsg, strlen(outMsg), 100);
                
                /* Print Erase Count */
                (void)strcpy(outMsg, "Signature: ");
                (void)memset(valBuf, 0, 16);
                app_itoa(sectorHeader.magicSignature, valBuf, 15);
                (void)strcat(outMsg, valBuf);
                
                (void)strcat(outMsg, "\r\nVersion: ");
                (void)memset(valBuf, 0, 16);
                app_itoa(sectorHeader.version, valBuf, 10);
                (void)strcat(outMsg, valBuf);
                
                (void)strcat(outMsg, "\r\nmax event: ");
                (void)memset(valBuf, 0, 16);
                app_itoa(sectorHeader.maxEvents, valBuf, 10);
                (void)strcat(outMsg, valBuf);
                
                (void)strcat(outMsg, "\r\nErase Count: ");
                (void)memset(valBuf, 0, 16);
                app_itoa(sectorHeader.eraseCount, valBuf, 12);
                (void)strcat(outMsg, valBuf);
                (void)strcat(outMsg, "\r\n---------------------\r\n");
                (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)outMsg, strlen(outMsg), 100);
            }

            while (readAddr < u32CurrentWriteAddress)
            {
                if (BSP_QSPI_Read((uint8_t *)&tempEvent, readAddr, LOG_ENTRY_SIZE) == QSPI_OK)
                {
                    /* Only process known sensor data points */
                    if (tempEvent.eventID == EVENT_ID_T_SENSOR_DATA_POINT)
                    {
                        /* Convert binary payload back to floats */
                        (void)memcpy((void*)&fTemp, (void*)&tempEvent.payload[0], sizeof(float));

                        /* Format output string: "Time: [tick] | T: [temp]" */
                        (void)memset(outMsg, 0, 128);
                        (void)memset(valBuf, 0, 16);
                        (void)strcpy(outMsg, "TS: ");
                        app_itoa(tempEvent.timestamp, valBuf, 10);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, " | T: ");
                        app_ftoa(fTemp, valBuf, 2);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, "\r\n");

                        /* Send directly to UART (bypassing queue for bulk dump) */
                        (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)outMsg, strlen(outMsg), 100);
                    }
                    if (tempEvent.eventID == EVENT_ID_H_SENSOR_DATA_POINT)
                    {
                        (void)memcpy(&fHum, &tempEvent.payload[0], sizeof(float));
                        /* Format output string: "Time: [tick] | T: [temp] | H: [hum]" */
                        (void)strcpy(outMsg, "TS: ");
                        app_itoa(tempEvent.timestamp, valBuf, 10);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, " | H: ");
                        app_ftoa(fHum, valBuf, 2);
                        (void)strcat(outMsg, valBuf);
                        (void)strcat(outMsg, "\r\n");

                        /* Send directly to UART (bypassing queue for bulk dump) */
                        (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)outMsg, strlen(outMsg), 100);
                    }
                }
                readAddr += LOG_ENTRY_SIZE;
                
                /* Add a tiny delay so we don't overwhelm the UART or starve the Heartbeat */
                vTaskDelay(pdMS_TO_TICKS(1)); 
            }
            (void)xSemaphoreGive(xUART1Mutex);
        }
        (void)xSemaphoreGive(xQSPIMutex);
    }

    appLoggerMessageEntry("--- FLASH LOG DUMP END ---\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
}

/* Internal function to find where we left off */
static void appLogger_ScanWriteHead(void)
{
    sStorageEvent_t tempEvent;
    /* Lock the hardware for the initial scan */
    if (xSemaphoreTake(xQSPIMutex, portMAX_DELAY) == pdTRUE)
    {
        bool headFound = false;
        uint32_t scanAddr = LOG_DATA_START; //first 16 bytes reserved for sector header data.
        /* Loop through flash in increments of our struct size */
        while (scanAddr < LOG_PARTITION_END)
        {
            //Read only 4 bytes of event ID size
            if (BSP_QSPI_Read((uint8_t *)&tempEvent, scanAddr, 4) == QSPI_OK)
            {
                /* 0xFFFFFFFF means this 'slot' is empty */
                if (tempEvent.timestamp == 0xFFFFFFFFU)
                {
                    u32CurrentWriteAddress = scanAddr;
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
            if (BSP_QSPI_Read((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
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
            BSP_QSPI_Write((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE);

            u32CurrentWriteAddress = LOG_DATA_START;
            appLoggerMessageEntry("Storage: Log Partition Full. Sector Erased & Wrapped.\r\n", 
                                    sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        else
        {
            char addrBuf[16];
            char msg[64] = {0};
            (void)strcpy(msg, "Storage: Resuming at addr: ");
            app_itoa((int32_t)u32CurrentWriteAddress, addrBuf, 10);
            (void)strncat(msg, addrBuf, sizeof(msg) - strlen(msg) - 1U);
            (void)strncat(msg, "\r\n", sizeof(msg) - strlen(msg) - 1U);
            appLoggerMessageEntry(msg, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        (void)xSemaphoreGive(xQSPIMutex);
    }
}

static void appLogger_storage_EventSectorErase(void)
{
    appLoggerMessageEntry("Storage: Starting Event log sector Erase (Wait ~5s)...\r\n", 
                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

    if (xSemaphoreTake(xQSPIMutex, portMAX_DELAY) == pdTRUE)
    {
        sLogSectorHeader_t currentHeader;
        /* Flash log sector is completely full, start at LOG_DATA_START */
        /* Read the old header to get the current erase count */
        //(void)memset(&currentHeader, 0, LOG_HEADER_SIZE);
        if (BSP_QSPI_Read((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
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
        BSP_QSPI_Write((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE);

        u32CurrentWriteAddress = LOG_DATA_START;
        (void)xSemaphoreGive(xQSPIMutex);
    }
}

static void appLogger_storage_BulkErase(void)
{
    appLoggerMessageEntry("Storage: Starting Full Chip Erase, system feels hang, but it don't (Wait ~25s)...\r\n", 
                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

    if (xSemaphoreTake(xQSPIMutex, portMAX_DELAY) == pdTRUE)
    {
        sLogSectorHeader_t currentHeader;

        /* Re-write the header with the new count */
        currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
        currentHeader.version = LOG_EVENT_SECTOR_VERSION;
        currentHeader.maxEvents = MAX_LOG_EVENTS;
        currentHeader.eraseCount = 1;
        

        /* 1. Initiate the Bulk Erase */
        if (BSP_QSPI_Erase_Chip() == QSPI_OK)
        {
            /* 2. Wait for completion without blocking the entire CPU */
            while (BSP_QSPI_GetStatus() == QSPI_BUSY)
            {
                /* Give 100ms to other tasks while hardware is busy */
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            BSP_QSPI_Write((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE);

            /* Reset the software write head */
            u32CurrentWriteAddress = LOG_DATA_START;
            appLoggerMessageEntry("Storage: Chip Erase Complete. Flash has Event Sector header only.\r\n", 
                                sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        else
        {
            appLoggerMessageEntry("Storage: Chip Erase FAILED!\r\n", 
                                sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }

        (void)xSemaphoreGive(xQSPIMutex);
    }
}

uint8_t appLogger_storage_Init(void)
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
            if(BSP_QSPI_Read((uint8_t*)&header, LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
            {
                if(header.magicSignature != STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE)
                {
                    /* Brand new chip or inconsistent data detected */
                    appLoggerMessageEntry("Storage: First-time boot detected. Erasing chip...\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    
                    appLogger_storage_BulkErase(); /* This resets u32CurrentWriteAddress to 0 */

                    /* Write the sector definition so we don't erase it next time */
                    //memset(&header, 0, LOG_HEADER_SIZE); //TO make sure there's no garbage on padding bytes
                    header.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
                    header.version = LOG_EVENT_SECTOR_VERSION;
                    header.maxEvents = MAX_LOG_EVENTS;
                    header.eraseCount = 1; // First erase
                    header.reserved = 0;

                    if(xSemaphoreTake(xQSPIMutex, portMAX_DELAY) == pdTRUE)
                    {
                        /* We write the signature at 0x0. Note: BulkErase already erased this sector. */
                        BSP_QSPI_Write((uint8_t*)&header, LOG_PARTITION_START, LOG_HEADER_SIZE);
                        sLogSectorHeader_t readBackSectorHeader;
                        if (BSP_QSPI_Read((uint8_t *)&readBackSectorHeader, LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
                        {
                            /* 3. Compare RAM vs. FLASH */
                            if (memcmp(&header, &readBackSectorHeader,LOG_HEADER_SIZE) == 0)
                            {
                                appLoggerMessageEntry("Storage: Sector header write Verified OK\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                            }
                            else
                            {
                                appLoggerMessageEntry("CRITICAL: Sector header Verification Failed! Bit-flip detected.\r\n", 
                                                    sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                            }
                        }

                        (void)xSemaphoreGive(xQSPIMutex);
                    }
                    
                    /* Start logging events immediately after the sector header data */
                    u32CurrentWriteAddress = LOG_HEADER_SIZE; 
                }
                else
                {
                    /* Normal Boot: Signature exists, just find where we left off */
                    appLoggerMessageEntry("Storage: Magic Signature Found. Scanning flash...\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    appLogger_ScanWriteHead();
                }
            }

            /* Compose Structured Binary Event */
            sEvent.timestamp = xTaskGetTickCount();
            sEvent.eventID   = EVENT_ID_QSPI_INIT_SUCCESS;
            sEvent.taskID    = TASK_ID_SYS_MANAGER; // System Manager Task ID
            sEvent.payload[0] = pInfo.EraseSectorSize;      //Size of sectors for the erase operation
            sEvent.payload[1] = pInfo.ProgPageSize;         //Size of pages for the program operation

            appLoggerEventEntry(&sEvent);
            /* Quick integer to string conversion for the size */
            app_itoa((int32_t)flashSizeMB, sizeBuf, 10);
            (void)strncat(qspiMsg, sizeBuf, sizeof(qspiMsg) - strlen(qspiMsg) - 1U);
            (void)strncat(qspiMsg, " MB\r\n", sizeof(qspiMsg) - strlen(qspiMsg) - 1U);

            appLoggerMessageEntry(qspiMsg, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            ret = (uint8_t)pdPASS;
        }
    }
    return ret;
}


void appLogger_Init(void)
{
    xPrintQueue = xQueueCreate(20, LOG_MESSAGE_SIZE);
    xEventQueue = xQueueCreate(20, LOG_ENTRY_SIZE);
    /* Create the Mutex before any task tries to use it */
    xQSPIMutex = xSemaphoreCreateMutex();
    xUART1Mutex = xSemaphoreCreateMutex();
    xCommandQueue = xQueueCreate(10, sizeof(uint8_t));

    /* Manually enable the interrupt in NVIC (BSP usually skips this) */
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0); 
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* cppcheck-suppress misra-c2012-8.7 */
void appLoggerEventEntry(const sStorageEvent_t *sEvent)
{
    // Send to queue
    (void)xQueueSend(xEventQueue, sEvent, 0);
}

void appLoggerMessageEntry( const char *pcMessage, 
                            sAppLoggerEventCode_t enumEventCodes)
{
    sAppLoggerMessage_t sLogMsg;
    sLogMsg.enumEventCode = enumEventCodes;
    
    (void)strncpy(sLogMsg.pcMessage, pcMessage, sizeof(sLogMsg.pcMessage) - 1U);
    sLogMsg.pcMessage[sizeof(sLogMsg.pcMessage) - 1U] = '\0';

    // Send to queue
    (void)xQueueSend(xPrintQueue, &sLogMsg, 0);
}

static BaseType_t LogMessageProcess(sAppLoggerMessage_t* sLogMsg)
{
    return xQueueReceive(xPrintQueue, sLogMsg, 0);
}

static BaseType_t LogEventProcess(sStorageEvent_t *sEvent)
{
    return xQueueReceive(xEventQueue, sEvent, 50);
}


static void FlushBufferToFlash(sStorageEvent_t *pBuffer, uint8_t eventCount)
{
    /* Calculate actual byte size to write */
    uint32_t writeSize = (uint32_t)eventCount * LOG_ENTRY_SIZE;
    sLogSectorHeader_t currentHeader;
    
    /* Hardware Protection */
    if (xSemaphoreTake(xQSPIMutex, portMAX_DELAY) == pdTRUE)
    {
        /* Boundary Check: If we wrap, we must update the Erase Count*/
        if ((u32CurrentWriteAddress + writeSize) >  LOG_PARTITION_END)
        {
            appLoggerMessageEntry("Storage: Erasing Event logging Sector...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            
            //Read existing header to get the current count */
            if (BSP_QSPI_Read((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE) == QSPI_OK)
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
            while (BSP_QSPI_GetStatus() == QSPI_BUSY) {
                vTaskDelay(pdMS_TO_TICKS(1)); 
            }

            //Re-write the updated header at the start of the clean sector */
            currentHeader.magicSignature = STORAGE_EVENT_SECTOR_MAGIC_SIGNATURE;
            currentHeader.version = LOG_EVENT_SECTOR_VERSION;
            currentHeader.maxEvents = MAX_LOG_EVENTS;
            // currentHeader.eraseCount is already incremented
            BSP_QSPI_Write((uint8_t*)&currentHeader, LOG_PARTITION_START, LOG_HEADER_SIZE);

            /* 4. Reset write head to the first data slot (Address 16) */
            u32CurrentWriteAddress = LOG_DATA_START;
            
            appLoggerMessageEntry("Storage: Sector Wrapped & Erase Count Updated\r\n", 
                                sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }

        /* 3. The Physical Write */
        /* Note: Most BSPs handle the 'Write Enable' internally inside the Write call */
        if (BSP_QSPI_Write((uint8_t *)pBuffer, u32CurrentWriteAddress, writeSize) == QSPI_OK)
        {
            static sStorageEvent_t readBackBuffer[16]; // Temporary buffer for verification
            /* Verification Step: Read back exactly what we just wrote */
            if (BSP_QSPI_Read((uint8_t *)readBackBuffer, u32CurrentWriteAddress, writeSize) == QSPI_OK)
            {
                /* Compare RAM vs. FLASH */
                if (memcmp((void*)pBuffer, (void*)readBackBuffer, writeSize) == 0)
                {
                    //appLoggerMessageEntry("Storage: Write Verified OK\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                }
                else
                {
                    appLoggerMessageEntry("CRITICAL: Storage Verification Failed! Bit-flip detected.\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    // In a real SSD, you would mark this block as 'Bad' here.
                }
            }
        }
    }
    u32CurrentWriteAddress += writeSize;
    /* 5. Release Resource */
    (void)xSemaphoreGive(xQSPIMutex);
} 


static void vAppLoggerManage (const sAppLoggerMessage_t *sLogMsg)
{
    if(sLogMsg->enumEventCode == sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE)
    {
        //Print to UART
        if (xSemaphoreTake(xUART1Mutex, portMAX_DELAY) == pdTRUE)
        {
            (void) HAL_UART_Transmit(&discoveryUART1, (const uint8_t *)sLogMsg->pcMessage,
                            (uint16_t)strlen(sLogMsg->pcMessage), DISCO_BOARD_UART_TIMEOUT_MS);
            (void)xSemaphoreGive(xUART1Mutex);
        }
    }
    //If this is info as per event table, log it to info section.
    else if(sLogMsg->enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_INFO)
    {

    }
    //If this is critical fault as per event table, log it to fault section.
    else if(sLogMsg->enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_FAULT)
    {

    }
    //If this is critical error as per event table, log it to errror section.
    else if(sLogMsg->enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_ERROR)
    {
        
    }//If this is critical warning as per event table, log it to warning section.
    else if(sLogMsg->enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_WARNING)
    {
        
    }
    else
    {
        //It shouldn't reaches here.    
    }
}

//TODO: MOdify it to prepend the EVENT CODE related string before the actual message
void vAppLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    sAppLoggerMessage_t sLogMsg;
    static sStorageEvent_t sEvent_PageBuffer[16]; //sStorageEvent_t is 16 bytes, so 16 x 16 = 256, which is size of page
    uint8_t eventCount = 0;

    for (;;)
    {
        //Wait FOREVER until a message arrives, If there's something to print in print queue then do it.
        //if (LogMessageProcess(&sLogMsg) == pdTRUE)
        //{
        /* 1. Drain the ENTIRE Print Queue first (Don't just read one) */
        while (LogMessageProcess(&sLogMsg) == pdPASS)
        {
            vAppLoggerManage(&sLogMsg);
        }

        /* Wait for an event, but with a timeout (e.g., 5 seconds) */
        if (LogEventProcess(&sEvent_PageBuffer[eventCount]) == pdPASS)
        {
            eventCount++;

            if(eventCount >= 16u)
            {
                /* Check 1: Write to flash if page is full */
                FlushBufferToFlash(sEvent_PageBuffer, eventCount);
                appLoggerMessageEntry("Storage: RAM Page full, Written to flash...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                eventCount = 0;
            }
        }
    }
}

// extern TaskHandle_t xAppLoggerTaskHandle;
// extern TaskHandle_t xAppCommandTaskHandle;
    
/* Internal function to print stack health for all tasks */
static void appLogger_CheckStackUsage(void)
{
    /* External handles from main.c - You'll need to make these visible */
    
    /* We can also get handles by name if they aren't global */
    TaskHandle_t xHeartbeatHndl = xTaskGetHandle("HeartBeatTask");
    TaskHandle_t xSysMgrHndl    = xTaskGetHandle("SysManagerTask");
    TaskHandle_t xSensorHndl    = xTaskGetHandle("SensorReadTask");

    TaskHandle_t handles[] = {xHeartbeatHndl, xSysMgrHndl, xSensorHndl, 
                             xAppLoggerTaskHandle, xAppCommandTaskHandle};
    
    /* Check the Global Heap */
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minEverFree = xPortGetMinimumEverFreeHeapSize();

    if(xSemaphoreTake(xUART1Mutex, portMAX_DELAY) == pdTRUE)
    {
        static char msg[128];
        (void)strcpy(msg,"\r\n--- STACK HIGH WATER MARKS (Words Remaining) ---\r\n");
        (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)msg, strlen(msg), 10);

        static char valBuf[16];
        for (int i = 0; i < 5; i++)
        {
            if (handles[i] != NULL)
            {
                const char* names[]    = {"Heartbeat", "SysManager", "SensorRead", "Logger", "Command"};
                UBaseType_t watermark = uxTaskGetStackHighWaterMark(handles[i]);
                //(void)memset(msg, 0, 128);
                (void)strcpy(msg, names[i]);
                (void)strcat(msg, ": ");
                app_itoa((uint32_t)watermark, valBuf, 10);
                (void)strcat(msg, valBuf);
                (void)strcat(msg, " words\r\n");
                
                /* Print directly to UART to avoid queueing during diagnostics */
                (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)msg, strlen(msg), 100);
            }
        }
        (void)strcpy(msg,"-----------------------------------------------\r\n");
        (void)strcat(msg,"-------------- HEAP STATUS (Bytes) -----------\r\n");
        
        (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)msg, strlen(msg), 100);
        (void)strcpy(msg, "Current Free Heap: ");
        app_itoa((uint32_t)freeHeap, valBuf, 10);
        (void)strcat(msg, valBuf);
        (void)strcat(msg, "\r\nMin Ever Free: ");
        app_itoa((uint32_t)minEverFree, valBuf, 10);
        (void)strcat(msg, valBuf);
        (void)strcat(msg, "\r\n---------------------------\r\n");

        (void)HAL_UART_Transmit(&discoveryUART1, (uint8_t*)msg, strlen(msg), 100);
    
        (void)xSemaphoreGive(xUART1Mutex);
    }
}

/* In stm32l4xx_it.c or main.c */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != NULL) 
    {
        if (huart->Instance == USART1)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            
            /* Send the received byte to the Command Task */
            (void)xQueueSendFromISR(xCommandQueue, (const void *)&g_rxChar, &xHigherPriorityTaskWoken);

            /* Re-enable the interrupt for the next character */
            (void)HAL_UART_Receive_IT(&discoveryUART1, (uint8_t *)&g_rxChar, 1u);

            /* Perform a context switch if a higher priority task was woken */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

static void Command_SendResponse(const char *pMsg)
{
    if (xSemaphoreTake(xUART1Mutex, 100U) == pdTRUE)
    {
        (void)HAL_UART_Transmit(&discoveryUART1, (const uint8_t*)pMsg, (uint16_t)strlen(pMsg), 100U);
        (void)xSemaphoreGive(xUART1Mutex);
    }
}

static void Command_ProcessChar(uint8_t rxChar)
{
    switch (rxChar)
    {
        case 'd':
            Command_SendResponse("Command: Triggering Flash Dump...\r\n");
            appLogger_DumpLogs();
            break;

        case 'n':
            Command_SendResponse("Command: Triggering Event sector erase...\r\n");
            appLogger_storage_EventSectorErase();
            break;

        case 'p':
            Command_SendResponse("Command: Triggering Bulk Erase...\r\n");
            appLogger_storage_BulkErase();
            break;

        case 'h':
            Command_SendResponse("\r\n--- Command Menu ---\r\n"
                                 "d: Dump Logs\r\n"
                                 "p: Bulk Erase\r\n"
                                 "s: Stack Health\r\n"
                                 "n: Event log sector erase\r\n");
            break;

        case 's':
            Command_SendResponse("Command: Checking Stack Health...\r\n");
            appLogger_CheckStackUsage();
            break;

        default:
            Command_SendResponse("Command: Did you typed command in small letter?...\r\n");
            
            break;
    }
}

void vCommandTask(void *pvParameters)
{
    uint8_t rxChar = 0;
    (void)pvParameters;

    /* Start the first interrupt-driven receive */
    (void)HAL_UART_Receive_IT(&discoveryUART1, &g_rxChar, 1);

    for (;;)
    {
        /* This task will SLEEP here until a key is pressed */
        if (xQueueReceive(xCommandQueue, &rxChar, portMAX_DELAY) == pdPASS)
        {
            Command_ProcessChar(rxChar);
        }
    }
}
