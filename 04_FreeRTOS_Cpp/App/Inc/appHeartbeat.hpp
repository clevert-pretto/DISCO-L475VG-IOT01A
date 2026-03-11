#ifndef APP_HEARTBEAT_HPP
#define APP_HEARTBEAT_HPP

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"

namespace FreeRTOS_Cpp
{
    class AppHeartbeat{

        public:
            // RAII Constructor via Dependency Injection
            AppHeartbeat(EventGroupHandle_t sysEvents, EventGroupHandle_t wdgEvents);
            
            static void HeartBeatTask(void *pvParameters);

        private:
            // Encapsulated RTOS handles
            EventGroupHandle_t _sysEvents;
            EventGroupHandle_t _wdgEvents;
            // Use constexpr inside the class instead of local const variables
            static constexpr uint32_t DELAY_INIT_MS        = 500U;
            static constexpr uint32_t DELAY_OPERATIONAL_MS = 1000U;
            static constexpr uint32_t DELAY_FAULT_MS       = 100U;
    };
}

#endif //APP_HEARTBEAT_HPP