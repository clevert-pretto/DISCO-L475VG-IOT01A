// standard includes
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// hardware includes
#include "../main.hpp"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// Application include
#include "appSensorRead.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"
#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "xtoa.hpp"

appSensorRead::appSensorRead(EventGroupHandle_t sysEvents, EventGroupHandle_t wdgEvents): _wdgEvents(wdgEvents), _sysEvents(sysEvents)
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
    if (BSP_TSENSOR_Init() != TSENSOR_OK) 
    {
        appLogger::logMessage("I2C Bus Error: TSENSOR Init Failed\r\n", 
            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else
    {
        ret |=  appSENSOR_TEMPERATURE;
    }

    //In ret set bit 1 for Temperature Sensor Init status
    if (BSP_HSENSOR_Init() != HSENSOR_OK) 
    {
        appLogger::logMessage("I2C Bus Error: HSENSOR Init Failed\r\n", 
            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    else
    {
        ret |=  appSENSOR_HUMIDITY;
    }

    // Manual Check: Is the I2C2 Clock actually enabled?
    if (!(RCC->APB1ENR1 & RCC_APB1ENR1_I2C2EN)) 
    {
        appLogger::logMessage("I2C Critical: Peripheral Clock is OFF\r\n", 
            sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
    }
    //In ret set bit 1 for Humidity Sensor Init status
    //ret |=  (appSENSOR_HUMIDITY & ((bool) BSP_HSENSOR_Init()));

    return ret;
} 

void appSensorRead::vSensorReadTask(void *pvParameters)
{
    float fTemp; 
    float fHumidity;
    //static char pcMessage[LOGGER_MESSAGE_STR_LEN] = {0}; // Temporary struct to fill
    //char tempBuf[5];
    sStorageEvent_t tEvent;
    sStorageEvent_t hEvent;
    char pcMessage[128] = {0};

    tEvent.eventID =  EVENT_ID_T_SENSOR_DATA_POINT;
    tEvent.taskID = TASK_ID_SENSOR_READ;
    hEvent.eventID =  EVENT_ID_H_SENSOR_DATA_POINT;
    hEvent.taskID = TASK_ID_SENSOR_READ;
    
    appSensorRead* self = static_cast<appSensorRead*>(pvParameters);

    for (;;)
    {
        EventBits_t uxBits = xEventGroupGetBits(self->_sysEvents);
        if(uxBits == EVENT_BIT_INIT_SUCCESS)
        {
            //Read temperature sensor
            fTemp = BSP_TSENSOR_ReadTemp();
            fHumidity = BSP_HSENSOR_ReadHumidity();
            
            tEvent.timestamp = xTaskGetTickCount();
            hEvent.timestamp = tEvent.timestamp;
            (void)memcpy(&tEvent.payload[0], &fTemp, sizeof(float));
            (void)memcpy(&hEvent.payload[0], &fHumidity, sizeof(float));
            appLogger::logEvent(&tEvent);
            appLogger::logEvent(&hEvent);

            #if(0)
            self->App_FormatSensorMsg(pcMessage, sizeof(pcMessage), "Temp", fTemp, "C");
            appLogger::logMessage(pcMessage, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            self->App_FormatSensorMsg(pcMessage, sizeof(pcMessage),"Humidity", fHumidity, "%");
            appLogger::logMessage(pcMessage, sAPPLOGGER_EVENT_CODE_PRINT_MESSAGE);
            #endif
        }
        // Add a delay so we don't spam the queue infinitely
        vTaskDelay(systemManager::appSensorReadSleepDuration);

        (void)xEventGroupSetBits(self->_wdgEvents, WATCHDOG_EVENT_BIT_TASK_SENSOR_READ);
    }
}