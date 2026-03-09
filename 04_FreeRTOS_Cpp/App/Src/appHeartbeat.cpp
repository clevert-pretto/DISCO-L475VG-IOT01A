
// Application include
#include "appHeartbeat.hpp"
#include "sysManager.hpp"

// standard includes
#include <string.h>
#include <stdint.h>

// hardware includes
#include "../main.hpp"



// Constructor initializes the private member handles
AppHeartbeat::AppHeartbeat(EventGroupHandle_t sysEvents, EventGroupHandle_t wdgEvents)
    : _sysEvents(sysEvents), _wdgEvents(wdgEvents) 
{

}


void AppHeartbeat::HeartBeatTask(void *pvParameters)
{
    AppHeartbeat* self = static_cast<AppHeartbeat*>(pvParameters);
    for (;;)
    {
        /* Read current state bits WITHOUT clearing them via our encapsulated handle */
        EventBits_t uxBits = xEventGroupGetBits(self->_sysEvents);

        if ((uxBits & EVENT_BIT_INIT_SUCCESS) != 0U)
        {
            BSP_LED_Toggle(LED2);
            vTaskDelay(pdMS_TO_TICKS(AppHeartbeat::DELAY_OPERATIONAL_MS));
        }
        else if((uxBits & EVENT_BIT_INIT_FAILED) != 0U)
        {
            BSP_LED_Toggle(LED2);
            vTaskDelay(pdMS_TO_TICKS(AppHeartbeat::DELAY_INIT_MS));
        }
        else
        {
            BSP_LED_Toggle(LED2);
            vTaskDelay(pdMS_TO_TICKS(AppHeartbeat::DELAY_FAULT_MS));
        }
        (void)xEventGroupSetBits(self->_wdgEvents, WATCHDOG_EVENT_BIT_TASK_HEARTBEAT);
    }
}