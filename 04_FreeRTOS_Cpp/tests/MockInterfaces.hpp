/* tests/MockInterfaces.hpp */
#ifndef MOCK_INTERFACES_HPP
#define MOCK_INTERFACES_HPP

#include <gmock/gmock.h>

/* Include the pure virtual interfaces */
#include "IRTOS.hpp"
#include "IHardware.hpp"
#include "ISensor.hpp"

namespace FreeRTOS_Cpp {

    /**
     * @brief Shared Mock for the RTOS Interface
     */
    class MockRtos : public IRTOS {
    public:
        MOCK_METHOD(void, delay, (uint32_t ms), (override));
        MOCK_METHOD(uint32_t, getTickCount, (), (override));
        MOCK_METHOD(void, registerTask, (void* handle, const char* name), (override));
        MOCK_METHOD(uint32_t, getRegisteredTaskCount, (), (override));
        MOCK_METHOD(bool, getRegisteredTaskInfo, (uint32_t index, const char**
            outName, void** outHandle), (override));
        MOCK_METHOD(uint32_t, getStackHighWaterMark, (void* handle), (override));
        
        MOCK_METHOD(uint32_t, getEventBits, (void* handle), (override));
        MOCK_METHOD(void, setEventBits, (void* handle, uint32_t bits), (override));
        MOCK_METHOD(void, clearEventBits, (void* handle, uint32_t bits), (override));
        MOCK_METHOD(uint32_t, WaitBits, (void* handle, uint32_t bits, bool clearOnExit, bool waitForAllBits, uint32_t tickTowait), (override));
        
        MOCK_METHOD(bool, takeMutex, (void* handle, uint32_t timeoutMs), (override));
        MOCK_METHOD(bool, giveMutex, (void* handle), (override));
        MOCK_METHOD(bool, giveSemaphoreFromISR, (void* handle, bool* higherPriorityTaskWoken), (override));
        
        MOCK_METHOD(bool, queueSend, (void* handle, const void* item, uint32_t timeoutMs), (override));
        MOCK_METHOD(bool, queueSendFromISR, (void* handle, const void* item, bool* higherPriorityTaskWoken), (override));
        MOCK_METHOD(bool, queueReceive, (void* handle, void* buffer, uint32_t timeoutMs), (override));
    };

    /**
     * @brief Shared Mock for the Hardware Interface
     */
    class MockHardware : public IHardware {
    public:
        MOCK_METHOD(void, toggleLed, (uint16_t ledId), (override));
        MOCK_METHOD(uint32_t, watchdog_Init, (void* handle), (override));
        MOCK_METHOD(uint8_t, watchdog_refresh, (void* handle), (override));
        MOCK_METHOD(void, printLog, (const uint8_t* data, uint16_t size), (override));
        MOCK_METHOD(void, startCommandReceiveIT, (volatile uint8_t* rxBuffer), (override));
        MOCK_METHOD(bool, storageInit, (uint32_t* flashSize, uint32_t* eraseSectorSize, uint32_t* progPageSize), (override));
        MOCK_METHOD(bool, storageRead, (uint8_t* pData, uint32_t readAddr, uint32_t size), (override));
        MOCK_METHOD(bool, storageWrite, (uint8_t* pData, uint32_t writeAddr, uint32_t size), (override));
        MOCK_METHOD(bool, storageEraseSector, (uint32_t sectorAddress), (override));
        MOCK_METHOD(bool, storageBulkErase, (), (override));
        MOCK_METHOD(bool, storageIsBusy, (), (override));
    };

    /**
     * @brief Shared Mock for the Sensor Interface
     */
    class MockSensor : public ISensor {
    public:
        MOCK_METHOD(bool, init, (), (override));
        MOCK_METHOD(float, read, (), (override));
    };

} // namespace FreeRTOS_Cpp

#endif // MOCK_INTERFACES_HPP