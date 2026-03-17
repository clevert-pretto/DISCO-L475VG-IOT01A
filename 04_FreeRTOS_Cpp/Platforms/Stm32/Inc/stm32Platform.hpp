#ifndef STM32_PLATFORM_HPP
#define STM32_PLATFORM_HPP

#include "IRTOS.hpp"
#include "IHardware.hpp"
#include "ISensor.hpp"
#include "appDefines.hpp"

namespace FreeRTOS_Cpp {

    class Stm32Rtos : public IRTOS {
    public:
        void delay(uint32_t ms) override;
        uint32_t getTickCount(void) override;
        uint32_t getEventBits(void* handle) override;
        void setEventBits(void* handle, uint32_t bits) override;
        void clearEventBits(void* handle, uint32_t bits) override;
        uint32_t WaitBits(void* handle, uint32_t bits, bool clearOnExit, bool waitForAllBits, uint32_t tickTowait) override;

        // --- Mutex / Semaphore Interface ---
        bool takeMutex(void* handle, uint32_t timeoutMs) override;
        bool giveMutex(void* handle) override;
        bool giveSemaphoreFromISR(void* handle, bool* higherPriorityTaskWoken) override;

        // --- Queue Interface ---
        bool queueSend(void* handle, const void* item, uint32_t timeoutMs) override;
        bool queueSendFromISR(void* handle, const void* item, bool* higherPriorityTaskWoken) override;
        bool queueReceive(void* handle, void* buffer, uint32_t timeoutMs) override;
    };

    class Stm32Hardware : public IHardware {
    public:
        void toggleLed(uint16_t ledId) override;
        uint32_t watchdog_Init(void * handle) override;
        uint8_t watchdog_refresh(void *handle) override;

        // --- UART / Logging Interface ---
        void printLog(const uint8_t* data, uint16_t size) override;
        void startCommandReceiveIT(volatile uint8_t* rxBuffer) override;

        // --- QSPI / Storage Interface ---
        bool storageInit(uint32_t* flashSize, uint32_t* eraseSectorSize, uint32_t* progPageSize) override;
        bool storageRead(uint8_t* pData, uint32_t readAddr, uint32_t size) override;
        bool storageWrite(uint8_t* pData, uint32_t writeAddr, uint32_t size) override;
        bool storageEraseSector(uint32_t sectorAddress) override;
        bool storageBulkErase() override;
        bool storageIsBusy() override;
    };

    class Stm32Sensor : public ISensor {
        public:
            // Function pointer types for BSP Init and Read
            typedef uint32_t (*BspInitFunc)(void);
            typedef float (*BspReadFunc)(void);

            Stm32Sensor(BspInitFunc initFn, BspReadFunc readFn) 
            : _initFn(initFn), _readFn(readFn) {}

            bool init() override;
            float read() override;

        PRIVATE_FOR_TEST:
            BspInitFunc _initFn;
            BspReadFunc _readFn;

    };

    extern Stm32Rtos realRtos;
    extern Stm32Hardware realHw;
    extern Stm32Sensor realTempSensor;
    extern Stm32Sensor realHumiditySensor;
}

#endif