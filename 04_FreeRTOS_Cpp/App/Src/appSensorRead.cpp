
#include <string.h>
#include "xtoa.hpp"
#include "appSensorRead.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"

namespace FreeRTOS_Cpp
{
    appSensorRead::appSensorRead(IRTOS* rtos, ISensor* tempSensor, ISensor* humiditySensor, void* sysEvents, void* wdgEvents)
        : _rtos(rtos),_tempSensor(tempSensor),_humiditySensor(humiditySensor), _sysEvents(sysEvents), _wdgEvents(wdgEvents)
    {

    }


    void appSensorRead::App_FormatSensorMsg(char *pDest, 
                            uint32_t destLen, const char *pLabel, 
                            float val, const char *pUnit)
    {
        char valBuf[16];
        
        /* 1. Ensure the destination starts empty (MISRA 19.1) */
        pDest[0] = '\0';

        /* 2. Safely concatenate label */
        (void)strncat(pDest, pLabel, destLen - 1U);
        (void)strncat(pDest, ": ", destLen - strlen(pDest) - 1U);

        /* 3. Convert float to string in temporary buffer */
        xtoa::app_ftoa(val, valBuf, (uint32_t)sizeof(valBuf));

        /* 4. Concatenate value and unit */
        (void)strncat(pDest, valBuf, destLen - strlen(pDest) - 1U);
        (void)strncat(pDest, " ", destLen - strlen(pDest) - 1U);
        (void)strncat(pDest, pUnit, destLen - strlen(pDest) - 1U);
        (void)strncat(pDest, "\r\n", destLen - strlen(pDest) - 1U);
    }

    uint8_t appSensorRead::appSensorRead_Init(void)
    {
        uint8_t ret = 0;
        //In ret set bit 0 for Temperature Sensor Init status
        if (_tempSensor->init() != true) 
        {
            appLogger::logMessage("I2C Bus Error: TSENSOR Init Failed\r\n", 
                sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        else
        {
            ret |=  appSENSOR_TEMPERATURE;
        }

        //In ret set bit 1 for Temperature Sensor Init status
        if (_humiditySensor->init() != true) 
        {
            appLogger::logMessage("I2C Bus Error: HSENSOR Init Failed\r\n", 
                sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
        }
        else
        {
            ret |=  appSENSOR_HUMIDITY;
        }

        return ret;
    } 

    void appSensorRead::vSensorReadTask(void *pvParameters)
    {
        appSensorRead* self = static_cast<appSensorRead*>(pvParameters);
        sStorageEvent_t tEvent;
        sStorageEvent_t hEvent;
        
        tEvent.eventID =  EVENT_ID_T_SENSOR_DATA_POINT;
        tEvent.taskID = TASK_ID_SENSOR_READ;
        hEvent.eventID =  EVENT_ID_H_SENSOR_DATA_POINT;
        hEvent.taskID = TASK_ID_SENSOR_READ;
    
        for (;;)
        {
            uint32_t uxBits = self->_rtos->getEventBits(self->_sysEvents);
            
            if((uxBits & EVENT_BIT_INIT_SUCCESS) != 0)
            {
                //Read temperature sensor
                self->_currentTemp = self->_tempSensor->read();
                self->_currentHumidity = self->_humiditySensor->read();

                hEvent.timestamp = tEvent.timestamp = self->_rtos->getTickCount();

                (void)memcpy(&tEvent.payload[0], &self->_currentTemp, sizeof(float));
                (void)memcpy(&hEvent.payload[0], &self->_currentHumidity, sizeof(float));

                if(self->bEnableTemperatureLogging)
                {
                    appLogger::logEvent(&tEvent);
                }
                if(self->bEnableHumidityLogging)
                {
                    appLogger::logEvent(&hEvent);
                }
                #if(0)
                char pcMessage[128] = {0};
                self->App_FormatSensorMsg(pcMessage, sizeof(pcMessage), "Temp", self->_currentTemp, "C");
                appLogger::logMessage(pcMessage, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                self->App_FormatSensorMsg(pcMessage, sizeof(pcMessage),"Humidity", self->_currentHumidity, "%");
                appLogger::logMessage(pcMessage, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
                #endif
            }
            // Add a delay so we don't spam the queue infinitely
            self->_rtos->delay(systemManager::appSensorReadSleepDuration);
            self->_rtos->setEventBits(self->_wdgEvents, WATCHDOG_BIT_SENSOR_READ);
        }
    }

    uint32_t appSensorRead::getTempSensorID(void)
    {
        return appSENSOR_TEMPERATURE;
    }
    
    uint32_t appSensorRead::getHumiditySensorID(void)
    {
        return appSENSOR_HUMIDITY;
    }
}