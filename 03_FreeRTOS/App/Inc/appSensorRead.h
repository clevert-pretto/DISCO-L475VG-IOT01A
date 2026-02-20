
#ifndef SENSOR_READ_H
#define SENSOR_READ_H

//Bit positions for each sensor
#define appSENSOR_TEMPERATURE 1u
#define appSENSOR_HUMIDITY    2u

#define LOG_SENSOR(buf, label, val, unit) \
    App_FormatSensorMsg((buf), (uint32_t)sizeof(buf), (label), (val), (unit))

uint8_t appSensorRead_Init(void);

void vSensorReadTask(void *pvParameters);

void App_FormatSensorMsg(char *pDest, uint32_t destLen, const char *pLabel, 
                         float val, const char *pUnit);
                         
#endif //SENSOR_READ_H