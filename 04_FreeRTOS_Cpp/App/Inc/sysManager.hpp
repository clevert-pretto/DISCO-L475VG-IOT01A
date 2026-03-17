#ifndef SYS_MANAGER_HPP
#define SYS_MANAGER_HPP

#include "appSensorRead.hpp" // Required for the instance pointer
#include "IRTOS.hpp"
#include "IHardware.hpp"
#include "appDefines.hpp"
namespace FreeRTOS_Cpp
{
    typedef enum {
        SYS_STATE_INIT_HARDWARE,  /* Initializing BSP */
        SYS_STATE_OPERATIONAL,    /* System is healthy and reading data */
        SYS_STATE_FAULT           /* A component failed; entering safe mode */
    } SystemState_t;

    class systemManager {
        
        public:
            systemManager(IRTOS* rtos, IHardware* hw, appSensorRead* sensorTask, void* sysEvents, void* wdgEvents, void* watchDog_Handle);
            
            static void systemManagerTask(void *pvParameters);

            static constexpr uint32_t  appSensorReadSleepDuration = 1000U;

        PRIVATE_FOR_TEST:
            IRTOS* _rtos; 
            IHardware* _hw;
            appSensorRead* _sensorTask;
            void* _sysEvents;
            void* _wdgEvents;
            void* _watchDog_Handle;
            SystemState_t currentState;
            void handleHardwareInit(void);
            void reportInitFailure(uint8_t sensorStatus, uint8_t qspiStatus);
    };

}
#endif //SYS_MANAGER_HPP