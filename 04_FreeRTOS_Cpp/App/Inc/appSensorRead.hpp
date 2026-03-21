
#ifndef SENSOR_READ_HPP
#define SENSOR_READ_HPP

#include "IRTOS.hpp"
#include "ISensor.hpp"
#include "appDefines.hpp"

namespace FreeRTOS_Cpp
{
    #define LOG_SENSOR(buf, label, val, unit) \
        App_FormatSensorMsg((buf), (uint32_t)sizeof(buf), (label), (val), (unit))

    class appSensorRead{
    
        public:
            appSensorRead(IRTOS* rtos, ISensor* tempSensor,
                  ISensor* humiditySensor, void* sysEvents, void* wdgEvents);

            uint8_t appSensorRead_Init(void);
            static void vSensorReadTask(void *pvParameters);

            uint32_t getTempSensorID(void);
            uint32_t getHumiditySensorID(void);

            float getCurrentTemp(void){return _currentTemp;};
            float getCurrentHumidity(void){return _currentHumidity;};

            void enableTemperatureLogging(void){bEnableTemperatureLogging = true;}
            void enableHumidityLogging(void){bEnableHumidityLogging = true;}
            void disableTemperatureLogging(void){bEnableTemperatureLogging = false;}
            void disableHumidityLogging(void){bEnableHumidityLogging = false;}
            
        PRIVATE_FOR_TEST:
            bool bEnableTemperatureLogging{false};
            bool bEnableHumidityLogging{false};
            IRTOS* _rtos;
            ISensor* _tempSensor;     // Hardware Abstracted
            ISensor* _humiditySensor; // Hardware Abstracted
            float _currentTemp;
            float _currentHumidity;
            void* _sysEvents;
            void* _wdgEvents;
            static constexpr uint32_t appSENSOR_TEMPERATURE =  (1u << 0); // Bit 0
            static constexpr uint32_t appSENSOR_HUMIDITY = (1u << 1); // Bit 1
            
            void App_FormatSensorMsg(char *pDest, uint32_t destLen, const char *pLabel, 
                            float val, const char *pUnit);
                        
    };
}
#endif //SENSOR_READ_H