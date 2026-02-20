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
    uint8_t initStatus = 0;
    (void)pvParameters;

    while(1) {
        switch(currentState) {
            case SYS_STATE_INIT_HARDWARE:
                appLoggerMessageEntry("Starting Hardware Init...\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                
                initStatus = appSensorRead_Init();
                
                if ((initStatus & (appSENSOR_TEMPERATURE | appSENSOR_HUMIDITY)) != 0U)
                {
                    currentState = SYS_STATE_OPERATIONAL;

                    /* Clear any fault bits and set Success */
                    (void)xEventGroupClearBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED | EVENT_BIT_FAULT_DETECTED);
                    (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_SUCCESS);
                } 
                else 
                {
                    if(!(initStatus & appSENSOR_TEMPERATURE))
                    {
                        appLoggerMessageEntry("Temperature Sensor Initialization Failed!\r\n", 
                                            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    }
                    else if(!(initStatus & appSENSOR_HUMIDITY))
                    {
                        appLoggerMessageEntry("Humidity Sensor Initialization Failed!\r\n", 
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