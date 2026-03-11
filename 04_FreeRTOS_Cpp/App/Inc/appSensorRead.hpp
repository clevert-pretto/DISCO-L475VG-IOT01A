
#ifndef SENSOR_READ_HPP
#define SENSOR_READ_HPP

#include "FreeRTOS.h"
#include "event_groups.h"

namespace FreeRTOS_Cpp
{
    //Bit positions for each sensor
    #define appSENSOR_TEMPERATURE 1u
    #define appSENSOR_HUMIDITY    2u

    #define LOG_SENSOR(buf, label, val, unit) \
        App_FormatSensorMsg((buf), (uint32_t)sizeof(buf), (label), (val), (unit))

    class appSensorRead{
    
        public:
            appSensorRead(EventGroupHandle_t sysEvents, EventGroupHandle_t wdgEvents);
            static uint8_t appSensorRead_Init(void);
            static void vSensorReadTask(void *pvParameters);

        private:
            EventGroupHandle_t _wdgEvents;
            EventGroupHandle_t _sysEvents;
            void App_FormatSensorMsg(char *pDest, uint32_t destLen, const char *pLabel, 
                            float val, const char *pUnit);
                        
    };
}
#endif //SENSOR_READ_H