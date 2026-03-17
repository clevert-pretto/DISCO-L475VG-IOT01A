#include "stm32Platform.hpp"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "appDefines.hpp"
#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_qspi.h"
#include "main.hpp" // For discoveryUART1 and QSPIHandle access

namespace FreeRTOS_Cpp 
{
    Stm32Rtos realRtos;
    Stm32Hardware realHw;
    Stm32Sensor realTempSensor(BSP_TSENSOR_Init, BSP_TSENSOR_ReadTemp);         
    Stm32Sensor realHumiditySensor(BSP_HSENSOR_Init, BSP_HSENSOR_ReadHumidity);

    // ==========================================
    // RTOS IMPLEMENTATION
    // ==========================================

    void Stm32Rtos::delay(uint32_t ms) 
    { 
        vTaskDelay(pdMS_TO_TICKS(ms)); 
    }

    uint32_t Stm32Rtos::getTickCount(void)
    {
        return xTaskGetTickCount();

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

    void Stm32Rtos::clearEventBits(void* handle, uint32_t bits)
    {
        if (handle != nullptr) 
        {
            xEventGroupClearBits(static_cast<EventGroupHandle_t>(handle), (bits)); 
        }
    }

    uint32_t Stm32Rtos::WaitBits(void* handle, uint32_t bits, bool clearOnExit, bool waitForAllBits, uint32_t tickTowait)
    {
        if (handle != nullptr) 
        {
            return xEventGroupWaitBits(
                static_cast<EventGroupHandle_t>(handle),
                static_cast<EventBits_t>(bits),
                static_cast<BaseType_t>(clearOnExit),     /* Clear bits on exit so they must report again */
                static_cast<BaseType_t>(waitForAllBits),  /* Wait for ALL bits */
                static_cast<TickType_t>(tickTowait)
            );
        }
        return 0U; // Fixed: Added return path for null handle
    }

    bool Stm32Rtos::takeMutex(void* handle, uint32_t timeoutMs) {
        if (handle == nullptr) return false;
        TickType_t ticks = (timeoutMs == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);
        return (xSemaphoreTake(static_cast<SemaphoreHandle_t>(handle), ticks) == pdTRUE);
    }

    bool Stm32Rtos::giveMutex(void* handle) {
        if (handle == nullptr) return false;
        return (xSemaphoreGive(static_cast<SemaphoreHandle_t>(handle)) == pdTRUE);
    }

    bool Stm32Rtos::giveSemaphoreFromISR(void* handle, bool* higherPriorityTaskWoken) {
        if (handle == nullptr || higherPriorityTaskWoken == nullptr) return false;
        BaseType_t rtosWoken = pdFALSE;
        BaseType_t res = xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(handle), &rtosWoken);
        *higherPriorityTaskWoken = (rtosWoken == pdTRUE);
        return (res == pdTRUE);
    }

    bool Stm32Rtos::queueSend(void* handle, const void* item, uint32_t timeoutMs) {
        if (handle == nullptr) return false;
        return (xQueueSend(static_cast<QueueHandle_t>(handle), item, pdMS_TO_TICKS(timeoutMs)) == pdPASS);
    }

    bool Stm32Rtos::queueSendFromISR(void* handle, const void* item, bool* higherPriorityTaskWoken) {
        if (handle == nullptr || higherPriorityTaskWoken == nullptr) return false;
        BaseType_t rtosWoken = pdFALSE;
        BaseType_t res = xQueueSendFromISR(static_cast<QueueHandle_t>(handle), item, &rtosWoken);
        *higherPriorityTaskWoken = (rtosWoken == pdTRUE);
        return (res == pdPASS);
    }

    bool Stm32Rtos::queueReceive(void* handle, void* buffer, uint32_t timeoutMs) {
        if (handle == nullptr) return false;
        TickType_t ticks = (timeoutMs == 0xFFFFFFFF) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);
        return (xQueueReceive(static_cast<QueueHandle_t>(handle), buffer, ticks) == pdPASS);
    }

    // ==========================================
    // HARDWARE IMPLEMENTATION
    // ==========================================

    void Stm32Hardware::toggleLed(uint16_t ledId) 
    { 
        if(ledId == HW_ID_STATUS_LED)
        {
            BSP_LED_Toggle((Led_TypeDef)LED2);  // Map abstract ID to real BSP define
        }
    }

    uint32_t Stm32Hardware::watchdog_Init(void* handle)
    {
        if(handle != nullptr)
        {
            // Cast to pointer type, as IWDG_handle is passed by address
            return HAL_IWDG_Init(static_cast<IWDG_HandleTypeDef*>(handle));
        }
        return 1; // Return error if handle is null
    }

    uint8_t Stm32Hardware::watchdog_refresh(void *handle)
    {
        if(handle != nullptr)
        {
            return static_cast<uint8_t>(HAL_IWDG_Refresh(static_cast<IWDG_HandleTypeDef*>(handle)));
        }
        return 1; // Return error if handle is null
    }

    void Stm32Hardware::printLog(const uint8_t* data, uint16_t size) {
        // Direct HAL call hidden from application logic
        HAL_UART_Transmit(&discoveryUART1, data, size, 100); 
    }

    void Stm32Hardware::startCommandReceiveIT(volatile uint8_t* rxBuffer) {
        HAL_UART_Receive_IT(&discoveryUART1, const_cast<uint8_t*>(rxBuffer), 1);
    }

    bool Stm32Hardware::storageInit(uint32_t* flashSize, uint32_t* eraseSectorSize, uint32_t* progPageSize) {
        QSPI_Info pInfo;
        if(BSP_QSPI_Init() == QSPI_OK) {
            if(BSP_QSPI_GetInfo(&pInfo) == QSPI_OK) {
                if (flashSize) *flashSize = pInfo.FlashSize;
                if (eraseSectorSize) *eraseSectorSize = pInfo.EraseSectorSize;
                if (progPageSize) *progPageSize = pInfo.ProgPageSize;
                return true;
            }
        }
        return false;
    }

    bool Stm32Hardware::storageRead(uint8_t* pData, uint32_t readAddr, uint32_t size) {
        return (BSP_QSPI_Read(pData, readAddr, size) == QSPI_OK);
    }

    bool Stm32Hardware::storageWrite(uint8_t* pData, uint32_t writeAddr, uint32_t size) {
        return (BSP_QSPI_Write(pData, writeAddr, size) == QSPI_OK);
    }

    bool Stm32Hardware::storageEraseSector(uint32_t sectorAddress) {
        return (BSP_QSPI_Erase_Sector(sectorAddress) == QSPI_OK);
    }

    bool Stm32Hardware::storageIsBusy() {
        return (BSP_QSPI_GetStatus() == QSPI_BUSY);
    }

    bool Stm32Hardware::storageBulkErase() {
        // Extracted directly from your old appLogger.cpp
        QSPI_CommandTypeDef s_command = {0};
        s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction       = 0x06; // WRITE ENABLE
        s_command.AddressMode       = QSPI_ADDRESS_NONE;
        s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
        s_command.DataMode          = QSPI_DATA_NONE;
        s_command.DummyCycles       = 0;
        s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
        s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
        s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

        if (HAL_QSPI_Command(&QSPIHandle, &s_command, 100) == HAL_OK) {
            s_command.Instruction = 0xC7; // CHIP ERASE
            return (HAL_QSPI_Command(&QSPIHandle, &s_command, 100) == HAL_OK);
        }
        return false;
    }

    // ==========================================
    // SENSOR IMPLEMENTATION
    // ==========================================

    bool Stm32Sensor::init()
    {
        return (_initFn() == 0);
    }

    float Stm32Sensor::read()
    {
        return _readFn();
    }
};