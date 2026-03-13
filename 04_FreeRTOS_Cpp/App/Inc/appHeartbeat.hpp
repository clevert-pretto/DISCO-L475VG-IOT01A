#ifndef APP_HEARTBEAT_HPP
#define APP_HEARTBEAT_HPP

#include "IRTOS.hpp"
#include "IHardware.hpp"

namespace FreeRTOS_Cpp
{
    class AppHeartbeat{

        public:
            // RAII Constructor via Dependency Injection
            AppHeartbeat(IRTOS* rtos, IHardware* hw, void* sysEvents, void* wdgEvents);
            
            // This method contains the LOGIC, making it 100% testable
            void update();
            
            // The Task remains as the RTOS entry point
            static void HeartBeatTask(void *pvParameters);

        private:
            IRTOS* _rtos;
            IHardware* _hw;
            // Encapsulated RTOS handles
            void* _sysEvents;
            void* _wdgEvents;
            // Use constexpr inside the class instead of local const variables
            static constexpr uint32_t DELAY_INIT_MS        = 500U;
            static constexpr uint32_t DELAY_OPERATIONAL_MS = 1000U;
            static constexpr uint32_t DELAY_FAULT_MS       = 100U;
    };
}

#endif //APP_HEARTBEAT_HPP