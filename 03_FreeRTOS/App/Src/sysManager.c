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

static void SysManager_ReportInitFailure(uint8_t sensorStatus, uint8_t qspiStatus)
{
    if ((sensorStatus & appSENSOR_TEMPERATURE) == 0U)
    {
        appLoggerMessageEntry("Temperature Sensor Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else if ((sensorStatus & appSENSOR_HUMIDITY) == 0U)
    {
        appLoggerMessageEntry("Humidity Sensor Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else if (qspiStatus == (uint8_t)pdFAIL)
    {
        appLoggerMessageEntry("QSPI Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else
    {
        appLoggerMessageEntry("Other Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
}

static void SysManager_HandleHardwareInit(void)
{
    appLoggerMessageEntry("Starting Hardware Init...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

    uint8_t sensorInitStatus = appSensorRead_Init();
    uint8_t qspiFlashInitStatus = appLogger_storage_Init();
    HAL_StatusTypeDef iwdgInitStatus = HAL_IWDG_Init(&IWDG_handle);

    /* Combined Success Check */
    uint8_t sensorMask = appSENSOR_TEMPERATURE | appSENSOR_HUMIDITY;
    if (((sensorInitStatus & sensorMask) == sensorMask) &&
        (qspiFlashInitStatus == (uint8_t)pdPASS)        &&
        (iwdgInitStatus == HAL_OK))
    {
        currentState = SYS_STATE_OPERATIONAL;
        appLoggerMessageEntry("Hardware Init successful\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        
        (void)xEventGroupClearBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED | EVENT_BIT_FAULT_DETECTED);
        (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_SUCCESS);
    }
    else
    {
        SysManager_ReportInitFailure(sensorInitStatus, qspiFlashInitStatus);
        appLoggerMessageEntry("Hardware Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        
        currentState = SYS_STATE_FAULT;
        (void)xEventGroupSetBits(xSystemEventGroup, EVENT_BIT_INIT_FAILED);
    }
}

void vSystemManagerTask(void *pvParameters) 
{
    (void)pvParameters;

    while(1) {
        switch(currentState) {
            case SYS_STATE_INIT_HARDWARE:
                SysManager_HandleHardwareInit();
            
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
             appLoggerMessageEntry("CRITICAL: Task Hang Detected! Starving Watchdog...\r\n", 
                                  sAPPLOGGER_EVENT_CODE_LOG_ERROR);
        }
    }
}