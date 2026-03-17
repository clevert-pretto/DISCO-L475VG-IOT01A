#ifndef I_HARDWARE_HPP
#define I_HARDWARE_HPP

#include <stdint.h>

namespace FreeRTOS_Cpp {
    class IHardware {
    public:
        virtual ~IHardware() = default;

        // --- Basic Hardware ---
        virtual void toggleLed(uint16_t ledId) = 0;
        virtual uint32_t watchdog_Init(void * handle) = 0;
        virtual uint8_t watchdog_refresh(void *handle) = 0;

        // --- UART / Logging Interface ---
        virtual void printLog(const uint8_t* data, uint16_t size) = 0;
        virtual void startCommandReceiveIT(volatile uint8_t* rxBuffer) = 0;

        // --- QSPI / Storage Interface ---
        // Uses pointers to return hardware specs to the application
        virtual bool storageInit(uint32_t* flashSize, uint32_t* eraseSectorSize, uint32_t* progPageSize) = 0;
        virtual bool storageRead(uint8_t* pData, uint32_t readAddr, uint32_t size) = 0;
        virtual bool storageWrite(uint8_t* pData, uint32_t writeAddr, uint32_t size) = 0;
        virtual bool storageEraseSector(uint32_t sectorAddress) = 0;
        virtual bool storageBulkErase() = 0;
        virtual bool storageIsBusy() = 0;
    };
}
#endif