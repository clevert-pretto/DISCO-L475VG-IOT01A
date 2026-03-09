// standard includes
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


// Kernel includes
#include "FreeRTOS.h"
#include "task.h"

// Application include
#include "appSensorRead.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"
#include "main.hpp"

systemManager::systemManager()
{

}

void systemManager::reportInitFailure(uint8_t sensorStatus, uint8_t qspiStatus)
{
    if ((sensorStatus & appSENSOR_TEMPERATURE) == 0U)
    {
        appLogger::logMessage("Temperature Sensor Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else if ((sensorStatus & appSENSOR_HUMIDITY) == 0U)
    {
        appLogger::logMessage("Humidity Sensor Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else if (qspiStatus == (uint8_t)pdFAIL)
    {
        appLogger::logMessage("QSPI Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else
    {
        appLogger::logMessage("Other Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
}

void systemManager::handleHardwareInit(void)
{
    appLogger::logMessage("Starting Hardware Init...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

    /* 1. Init Watchdog FIRST so storageBulkErase can safely pet it */
    HAL_StatusTypeDef iwdgInitStatus = HAL_IWDG_Init(&IWDG_handle);

    /* 2. Init Sensors */
    uint8_t sensorInitStatus = appSensorRead::appSensorRead_Init();

    /* 3. Init Storage */
    appLogger::storageInitStatus = appLogger::instance->storageInit();
    
    /* Combined Success Check */
    uint8_t sensorMask = appSENSOR_TEMPERATURE | appSENSOR_HUMIDITY;
    if (((sensorInitStatus & sensorMask) == sensorMask) &&
        (appLogger::storageInitStatus == (uint8_t)pdPASS)        &&
        (iwdgInitStatus == HAL_OK))
    {
        currentState = SYS_STATE_OPERATIONAL;
        appLogger::logMessage("Hardware Init successful\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        
        (void)xEventGroupClearBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED | EVENT_BIT_FAULT_DETECTED);
        (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_SUCCESS);
    }
    else
    {
        reportInitFailure(sensorInitStatus, appLogger::storageInitStatus);
        appLogger::logMessage("Hardware Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        
        currentState = SYS_STATE_FAULT;
        (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED);
    }
}

void systemManager::systemManagerTask(void *pvParameters) 
{
    systemManager* self = static_cast<systemManager*>(pvParameters);
    
    self->currentState = SYS_STATE_INIT_HARDWARE;

    while(1) {
        switch(self->currentState) {
            case SYS_STATE_INIT_HARDWARE:
                self->handleHardwareInit();
            
                break;

            case SYS_STATE_OPERATIONAL:
                
                //vTaskDelay(SYS_MANAGER_SLEEP_DURATION); // Run check loop every 1s
                break;

            case SYS_STATE_FAULT:
                //vTaskDelay(SYS_MANAGER_SLEEP_DURATION);
                break;

            default:
                /* Defensive: handle unknown states */
                break;
        }

        EventBits_t uxBits = xEventGroupWaitBits(
            xWatchdogEventGroup,
            WATCHDOG_MANDATORY_TASKS_BITMASK,
            pdTRUE,        /* Clear bits on exit so they must report again */
            pdTRUE,        /* Wait for ALL bits */
            pdMS_TO_TICKS(IWDG_TIMEOUT_ms) 
        );

        if ((uxBits & WATCHDOG_MANDATORY_TASKS_BITMASK) == WATCHDOG_MANDATORY_TASKS_BITMASK) {
            /* All tasks are healthy! Pet the hardware dog. */
            HAL_IWDG_Refresh(&IWDG_handle);
        } else {
            /* * If we reach here, at least one task is hung! 
             * Do NOT refresh the IWDG. Let the hardware reset the MCU.
             */
             appLogger::logMessage("CRITICAL: Task Hang Detected! Starving Watchdog...\r\n", 
                                  sAPPLOGGER_EVENT_CODE_LOG_ERROR);
        }
    }
}