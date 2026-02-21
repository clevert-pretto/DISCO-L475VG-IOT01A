// standard includes
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


// Kernel includes
#include "FreeRTOS.h"
#include "task.h"

// Application include
#include "appSensorRead.h"
#include "appLogger.h"
#include "sysManager.h"
#include "main.h"

SystemState_t currentState = SYS_STATE_INIT_HARDWARE;

void vSystemManagerTask(void *pvParameters) 
{
    uint8_t sensorInitStatus = 0;
    uint8_t QSPIFlashInitStatus = 0;
    (void)pvParameters;

    while(1) {
        switch(currentState) {
            case SYS_STATE_INIT_HARDWARE:
                appLoggerMessageEntry("Starting Hardware Init...\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                
                sensorInitStatus = appSensorRead_Init();
                QSPIFlashInitStatus = appLogger_storage_Init();
                if (((sensorInitStatus & (appSENSOR_TEMPERATURE | appSENSOR_HUMIDITY)) == 
                         (appSENSOR_TEMPERATURE | appSENSOR_HUMIDITY)) &&
                            (QSPIFlashInitStatus == (uint8_t)pdPASS))
                {
                    currentState = SYS_STATE_OPERATIONAL;
                    
                    appLoggerMessageEntry("Hardware Init successful\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                                        
                    /* Clear any fault bits and set Success */
                    (void)xEventGroupClearBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED | EVENT_BIT_FAULT_DETECTED);
                    (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_SUCCESS);
                } 
                else 
                {
                    if(!(sensorInitStatus & appSENSOR_TEMPERATURE))
                    {
                        appLoggerMessageEntry("Temperature Sensor Initialization Failed!\r\n", 
                                            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    }
                    else if(!(sensorInitStatus & appSENSOR_HUMIDITY))
                    {
                        appLoggerMessageEntry("Humidity Sensor Initialization Failed!\r\n", 
                                            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    }
                    else if((QSPIFlashInitStatus == (uint8_t)pdFAIL))
                    {
                        appLoggerMessageEntry("QSPI Initialization Failed!\r\n", 
                                            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    }
                    else
                    {
                        appLoggerMessageEntry("Other Initialization Failed!\r\n", 
                                            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    }
                    //TODO: Move to error section later
                    appLoggerMessageEntry("Hardware Initialization Failed!\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    currentState = SYS_STATE_FAULT;
                    
                    (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED);
                }
                break;

            case SYS_STATE_OPERATIONAL:
                
                vTaskDelay(SYS_MANAGER_SLEEP_DURATION); // Run check loop every 1s
                break;

            case SYS_STATE_FAULT:
                vTaskDelay(SYS_MANAGER_SLEEP_DURATION);
                break;

            default:
                /* Defensive: handle unknown states */
                break;
        }
    }
}