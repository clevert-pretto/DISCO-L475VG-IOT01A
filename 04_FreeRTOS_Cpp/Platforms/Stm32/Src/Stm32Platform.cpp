#include "stm32Platform.hpp"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "appDefines.hpp"
#include "stm32l475e_iot01.h"

namespace FreeRTOS_Cpp 
{
    Stm32Rtos realRtos;
    Stm32Hardware realHw;

    void Stm32Rtos::delay(uint32_t ms) 
    { 
        vTaskDelay(pdMS_TO_TICKS(ms)); 
    }

    uint32_t Stm32Rtos::getEventBits(void* handle)
    { 
        if (handle == nullptr) 
            return 0U;
        return xEventGroupGetBits(static_cast<EventGroupHandle_t>(handle)); 
    }

    void Stm32Rtos::setEventBits(void* handle, uint32_t bits)
    {
        if (handle != nullptr) 
        {
            xEventGroupSetBits(static_cast<EventGroupHandle_t>(handle), static_cast<EventBits_t>(bits)); 
        }
    }

    void Stm32Hardware::toggleLed(uint16_t ledId) 
    { 
        if(ledId == HW_ID_STATUS_LED)
        {
            BSP_LED_Toggle((Led_TypeDef)LED2);  // Map abstract ID to real BSP define
        }
    }

};