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

void HeartBeatTask(void *pvParameters)
{
    (void)pvParameters;
    const uint32_t heartbeat_delay_ms = 1000U;
    for (;;)
    {
        BSP_LED_Toggle(LED2);
        vTaskDelay(pdMS_TO_TICKS(heartbeat_delay_ms));
    }
}