// standard includes
#include <string.h>
#include <stdint.h>

// hardware includes
#include "../main.h"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// Application include
#include "appLogger.h"
#include "sysManager.h"

//extern UART_HandleTypeDef discoveryUART1;
static QueueHandle_t xPrintQueue;

void appLogger_Init(void)
{
    xPrintQueue = xQueueCreate(5, sizeof(sAppLoggerMessage_t));
}

void appLoggerMessageEntry( const char *pcMessage, 
                            sAppLoggerEventCode_t enumEventCodes)
{
    sAppLoggerMessage_t sLogMsg;
    sLogMsg.enumEventCode = enumEventCodes;
    
    (void)strncpy(sLogMsg.pcMessage, pcMessage, sizeof(sLogMsg.pcMessage) - 1U);
    sLogMsg.pcMessage[sizeof(sLogMsg.pcMessage) - 1U] = '\0';

    // Send to queue
    (void)xQueueSend(xPrintQueue, &sLogMsg, pdMS_TO_TICKS(MAX_DELAY_FOR_LOGGER_QUEUEms));
}

static BaseType_t LogMessageProcess(sAppLoggerMessage_t* sLogMsg)
{
    return xQueueReceive(xPrintQueue, sLogMsg, pdMS_TO_TICKS(MAX_DELAY_FOR_LOGGER_QUEUEms));
}

void vAppLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    sAppLoggerMessage_t sLogMsg;
    for (;;)
    {
        //Wait FOREVER until a message arrives, If there's something to print in print queue then do it.
        if (LogMessageProcess(&sLogMsg) == pdTRUE)
        {
            if(sLogMsg.enumEventCode == sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE)
            {
                //Print to UART
                (void) HAL_UART_Transmit(&discoveryUART1, (const uint8_t *)sLogMsg.pcMessage,
                                (uint16_t)strlen(sLogMsg.pcMessage), (uint32_t)100);
            }
            //If this is critical fault as per event table, log it to fault section.
            else if(sLogMsg.enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_FAULT)
            {

            }
            //If this is critical error as per event table, log it to errror section.
            else if(sLogMsg.enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_ERROR)
            {
                
            }//If this is critical warning as per event table, log it to warning section.
            else if(sLogMsg.enumEventCode == sAPPLOGGER_EVENT_CODE_LOG_WARNING)
            {
                
            }
            else
            {
                //It shouldn't reaches here.    
            }
        }
    }
}
