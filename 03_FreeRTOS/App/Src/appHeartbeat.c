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
#include "appHeartbeat.h"
#include "sysManager.h"


void HeartBeatTask(void *pvParameters)
{
    (void)pvParameters;
    const uint32_t heartbeat_initHW_delay_ms = 500U;
    const uint32_t heartbeat_operational_delay_ms = 1000U;
    const uint32_t heartbeat_fault_delay_ms = 100U;
    for (;;)
    {
        if(currentState == SYS_STATE_INIT_HARDWARE) // Blink at 0.5 sec interval
        {
            BSP_LED_Toggle(LED2);
            vTaskDelay(pdMS_TO_TICKS(heartbeat_initHW_delay_ms));
        }
        else if(currentState == SYS_STATE_OPERATIONAL) // Blink at 1 sec interval
        {
            BSP_LED_Toggle(LED2);
            vTaskDelay(pdMS_TO_TICKS(heartbeat_operational_delay_ms));
        }
        else // In fault blink faster at 0.1 sec interval
        {
            BSP_LED_Toggle(LED2);
            vTaskDelay(pdMS_TO_TICKS(heartbeat_fault_delay_ms));
        }
    }
}