// standard includes
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Application include
#include "appSensorRead.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"

namespace FreeRTOS_Cpp
{
    systemManager::systemManager(IRTOS* rtos, IHardware* hw, appSensorRead* sensorTask, 
                                 void* sysEvents, void* wdgEvents, void* watchDog_Handle)
        : _rtos(rtos), _hw(hw), _sensorTask(sensorTask), 
          _sysEvents(sysEvents), _wdgEvents(wdgEvents), 
          _watchDog_Handle(watchDog_Handle), currentState(SYS_STATE_INIT_HARDWARE)
    {

    }

    /* sysManager.cpp */

    void systemManager::reportInitFailure(uint8_t sensorStatus, uint8_t qspiStatus)
    {
        // Use independent 'if' blocks to report all current failures
        if ((sensorStatus & _sensorTask->getTempSensorID()) == 0U)
        {
            appLogger::logMessage("Temperature Sensor Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        
        // FIXED: Corrected ID check for Humidity
        if ((sensorStatus & _sensorTask->getHumiditySensorID()) == 0U)
        {
            appLogger::logMessage("Humidity Sensor Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }

        if (qspiStatus == 0U) // pdFAIL is usually 0
        {
            appLogger::logMessage("QSPI Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
    }

    void systemManager::handleHardwareInit(void)
    {
        appLogger::logMessage("Starting Hardware Init...\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);

        /* 1. Init Watchdog FIRST so storageBulkErase can safely pet it */
        uint32_t iwdgInitStatus = _hw->watchdog_Init(_watchDog_Handle);

        /* The Power-On Reset (POR) Delay 
           Give the physical I2C sensors 200ms to boot up and stabilize 
           their logic before we start probing the I2C bus. */
        _rtos->delay(200);
        _hw->watchdog_refresh(_watchDog_Handle); // Pet the dog while we wait!

        /* 2. Init Sensors */
        uint8_t sensorInitStatus = _sensorTask->appSensorRead_Init();

        /* 3. Init Storage */
        appLogger::storageInitStatus = appLogger::instance->storageInit();
        
        /* Combined Success Check */
        uint32_t sensorMask = (_sensorTask->getTempSensorID() | _sensorTask->getHumiditySensorID());
        
        if (((sensorInitStatus & sensorMask) == sensorMask) &&
            (appLogger::storageInitStatus) && (!iwdgInitStatus))
        {
            currentState = SYS_STATE_OPERATIONAL;
            appLogger::logMessage("Hardware Init successful\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            
            _rtos->clearEventBits(_sysEvents, EVENT_BIT_INIT_FAILED | EVENT_BIT_FAULT_DETECTED);
            _rtos->setEventBits(_sysEvents, EVENT_BIT_INIT_SUCCESS);
        }
        else
        {
            reportInitFailure(sensorInitStatus, appLogger::storageInitStatus);
            appLogger::logMessage("Hardware Initialization Failed!\r\n", sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            
            currentState = SYS_STATE_FAULT;
            _rtos->setEventBits(_sysEvents, EVENT_BIT_INIT_FAILED);
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

            uint32_t uxBits = self->_rtos->WaitBits(
                self->_wdgEvents,
                WATCHDOG_MANDATORY_BITMASK,
                true,        /* Clear bits on exit so they must report again */
                true,        /* Wait for ALL bits */
                (IWDG_TIMEOUT_ms) 
            );

            if ((uxBits & WATCHDOG_MANDATORY_BITMASK) == WATCHDOG_MANDATORY_BITMASK) {
                /* All tasks are healthy! Pet the hardware dog. */
                self->_hw->watchdog_refresh(self->_watchDog_Handle);
            } else {
                /* * If we reach here, at least one task is hung! 
                * Do NOT refresh the IWDG. Let the hardware reset the MCU.
                */
                appLogger::logMessage("CRITICAL: Task Hang Detected! Starving Watchdog...\r\n", 
                                    sAPPLOGGER_EVENT_CODE_LOG_ERROR);
            }
        }
    }
}