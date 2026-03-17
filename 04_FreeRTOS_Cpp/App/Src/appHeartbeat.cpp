
// Application include
#include "appHeartbeat.hpp"

namespace FreeRTOS_Cpp
{

    // Constructor initializes the private member handles
    AppHeartbeat::AppHeartbeat(IRTOS* rtos, IHardware* hw, void* sysEvents, void* wdgEvents)
        : _rtos(rtos), _hw(hw), _sysEvents(sysEvents), _wdgEvents(wdgEvents) 
    {

    }

    void AppHeartbeat::update() {
        // Logic uses the interface, not the raw RTOS/HAL calls
        uint32_t uxBits = _rtos->getEventBits(_sysEvents);

        if ((uxBits & EVENT_BIT_INIT_SUCCESS) != 0U) {
            _hw->toggleLed(HW_ID_STATUS_LED);
            _rtos->delay(DELAY_OPERATIONAL_MS);
        } else if ((uxBits & EVENT_BIT_INIT_FAILED) != 0U) {
            _hw->toggleLed(HW_ID_STATUS_LED);
            _rtos->delay(DELAY_INIT_MS);
        } else {
            _hw->toggleLed(HW_ID_STATUS_LED);
            _rtos->delay(DELAY_FAULT_MS);
        }
        
        _rtos->setEventBits(_wdgEvents, WATCHDOG_BIT_HEARTBEAT);
    }

    void AppHeartbeat::HeartBeatTask(void *pvParameters)
    {
        AppHeartbeat* self = static_cast<AppHeartbeat*>(pvParameters);
        for (;;)
        {
            self->update();
        }
    }

}