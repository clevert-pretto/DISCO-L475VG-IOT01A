#ifndef I_RTOS_HPP
#define I_RTOS_HPP

#include <stdint.h>

namespace FreeRTOS_Cpp {
    class IRTOS {
    public:
        virtual ~IRTOS() = default;

        // delay, tick
        virtual void delay(uint32_t ms) = 0;
        virtual uint32_t getTickCount(void) = 0;

        //Events
        virtual uint32_t getEventBits(void* handle) = 0;
        virtual void setEventBits(void* handle, uint32_t bits) = 0;
        virtual void clearEventBits(void* handle, uint32_t bits) = 0;
        virtual uint32_t WaitBits(void* handle, uint32_t bits, bool clearOnExit, bool waitForAllBits, uint32_t tickTowait) = 0;
        
        // --- Mutex / Semaphore Interface ---
        virtual bool takeMutex(void* handle, uint32_t timeoutMs) = 0;
        virtual bool giveMutex(void* handle) = 0;
        virtual bool giveSemaphoreFromISR(void* handle, bool* higherPriorityTaskWoken) = 0;

        // --- Queue Interface ---
        virtual bool queueSend(void* handle, const void* item, uint32_t timeoutMs) = 0;
        virtual bool queueSendFromISR(void* handle, const void* item, bool* higherPriorityTaskWoken) = 0;
        virtual bool queueReceive(void* handle, void* buffer, uint32_t timeoutMs) = 0;
    };
}
#endif