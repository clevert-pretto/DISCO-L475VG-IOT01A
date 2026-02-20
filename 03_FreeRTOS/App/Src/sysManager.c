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
                //TODO : Add other sensors bit check here
                if (initStatus == (uint8_t)pdPASS)
                {
                    currentState = SYS_STATE_OPERATIONAL;
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

                    }
                    //TODO: Move to error section later
                    appLoggerMessageEntry("Hardware Initialization Failed!\r\n", 
                                        sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                    currentState = SYS_STATE_FAULT;
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