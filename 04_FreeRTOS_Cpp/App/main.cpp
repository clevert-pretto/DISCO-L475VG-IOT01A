// standard includes
#include <string.h>

// hardware includes
#include "main.hpp"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

//Application includes
#include "stm32Platform.hpp"
#include "appHeartbeat.hpp"
#include "appSensorRead.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"
#include "appDefines.hpp"

namespace FreeRTOS_Cpp
{
    /*===================== FreeRTOS Hook Prototypes ============================ */
        
    extern "C"
        {
        void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, 
                                            StackType_t **ppxIdleTaskStackBuffer, 
                                            size_t  *pulIdleTaskStackSize);

        void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
                                            StackType_t **ppxTimerTaskStackBuffer, 
                                            size_t  *pulTimerTaskStackSize);

        void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
        void vApplicationMallocFailedHook(void);
    }
    static void SystemClock_Config(void);
    void assert_failed(uint8_t *file, uint32_t line);

    /* ============================ COMMON RESOURCES ============================ */
    #ifdef __cplusplus
    extern "C"{
    #endif
        UART_HandleTypeDef discoveryUART1;
        //QSPI_HandleTypeDef QSPIHandle;
        IWDG_HandleTypeDef IWDG_handle;
    #ifdef __cplusplus
    }
    #endif

    EventGroupHandle_t xSystemEventGroup;
    EventGroupHandle_t xWatchdogEventGroup;

    /* Static Memory for Event Groups (Zero Heap usage) */
    static StaticEventGroup_t xSystemEventGroupBuffer;
    static StaticEventGroup_t xWatchdogEventGroupBuffer;
    
    /* Static Memory for Logger Queues */
    static StaticQueue_t xPrintQueueBuffer;
    static uint8_t ucPrintQueueStorage[FreeRTOS_Cpp::appLogger::QUEUE_LEN * LOG_MESSAGE_SIZE];

    static StaticQueue_t xEventQueueBuffer;
    static uint8_t ucEventQueueStorage[FreeRTOS_Cpp::appLogger::QUEUE_LEN * LOG_ENTRY_SIZE];
    
    static StaticQueue_t xCommandQueueBuffer;
    static uint8_t ucCmmandQueueStorage[10U * COMMAND_ENTRY_SIZE];

    /* Static Memory for Logger Mutexes & Semaphores */
    static StaticSemaphore_t xUartMutexBuffer;
    static StaticSemaphore_t xQspiMutexBuffer;
    static StaticSemaphore_t xEraseCompleteMutexBuffer;

    TaskHandle_t xvSensorReadTaskHandle = NULL;     //For Sensor read task
    TaskHandle_t xsystemManagerTaskHandle = NULL;   //For System manager task
    TaskHandle_t xHeartBeatTaskHandle = NULL;       //For heart beat task
    TaskHandle_t xAppLoggerTaskHandle = NULL;       //For App Logger task
    TaskHandle_t xAppCommandTaskHandle = NULL;      //For Command task

    // Hardware Setup
    /**
     * @brief  System Clock Configuration
     *         The system Clock is configured as follow :
     *            System Clock source            = PLL (MSI)
     *            SYSCLK(Hz)                     = 80000000
     *            HCLK(Hz)                       = 80000000
     *            AHB Prescaler                  = 1
     *            APB1 Prescaler                 = 1
     *            APB2 Prescaler                 = 1
     *            MSI Frequency(Hz)              = 4000000
     *            PLL_M                          = 1
     *            PLL_N                          = 40
     *            PLL_R                          = 2
     *            PLL_P                          = 7
     *            PLL_Q                          = 4
     *            Flash Latency(WS)              = 4
     * @param  None
     * @retval None
     */
    static void SystemClock_Config(void)
    {
        RCC_ClkInitTypeDef RCC_ClkInitStruct;
        RCC_OscInitTypeDef RCC_OscInitStruct;
        
        /* MSI is enabled after System reset, activate PLL with MSI as source */
        RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
        RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
        RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_6;
        RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_MSI;

        /* To get 5 seconds: 32kHz / 64 = 500Hz. 
        500Hz * 2500 ticks = 5000ms (5 seconds) */
        IWDG_handle.Instance        = IWDG;
        IWDG_handle.Init.Prescaler  = IWDG_PRESCALER_64; // 32Khz/64 = 0.5Khz (0.5ms per tick) 
        IWDG_handle.Init.Reload     = 2500; //5 second timeout
        IWDG_handle.Init.Window     = IWDG_WINDOW_DISABLE;
        
        /* PLL Math for 80MHz System Clock:
        Input (MSI) = 4 MHz
        PLLM (Div)  = 1   -> 4 MHz / 1 = 4 MHz (VCO Input)
        PLLN (Mul)  = 10  -> 4 MHz * 40 = 160 MHz (VCO Output)
        PLLR (Div)  = 2   -> 160 MHz / 2 = 80 MHz (SYSCLK)
        */

        RCC_OscInitStruct.PLL.PLLM = 1;
        RCC_OscInitStruct.PLL.PLLN = 40;
        RCC_OscInitStruct.PLL.PLLR = 2;
        RCC_OscInitStruct.PLL.PLLP = 7;
        RCC_OscInitStruct.PLL.PLLQ = 4;

        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        {
            /* Initialization Error */
            while (1)
            {

            }
        }

        /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
        clocks dividers */
        RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
        RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        {
            /* Initialization Error */
            while (1)
            {

            }
        }
    }

    static void UART1_Init(void)
    {
        discoveryUART1.Instance = USART1;
        discoveryUART1.Init.BaudRate = 115200;
        discoveryUART1.Init.WordLength = UART_WORDLENGTH_8B;
        discoveryUART1.Init.StopBits = UART_STOPBITS_1;
        discoveryUART1.Init.Parity = UART_PARITY_NONE;
        discoveryUART1.Init.Mode = UART_MODE_TX_RX;
        discoveryUART1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        discoveryUART1.Init.OverSampling = UART_OVERSAMPLING_16;
        if (HAL_UART_Init(&discoveryUART1) != HAL_OK)
        {
            while(1){}
        }
    }



    /* =========================== COMMON RESOURCES END ========================= */
    /* ============================ TASK FUNCTION =============================== */
    
    /* ========================== TASK FUNCTION END ============================= */

    /* =============================== TASK HOOKS =============================== */
    void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                        StackType_t  **ppxIdleTaskStackBuffer,
                                        size_t *pulIdleTaskStackSize)
    {
        static StaticTask_t xIdleTaskTCB;
        static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];
        
        *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
        *ppxIdleTaskStackBuffer = uxIdleTaskStack;
        *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
    }

    void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, 
                                        StackType_t **ppxTimerTaskStackBuffer,
                                        size_t *pulTimerTaskStackSize)
    {
        static StaticTask_t xTimerTaskTCB;
        static StackType_t  uxTimerTaskStack[configMINIMAL_STACK_SIZE];

        *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
        *ppxTimerTaskStackBuffer = uxTimerTaskStack;
        *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
    }

    /* Hook called when a task overflows its stack */
    // cppcheck-suppress unusedFunction
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
    {
        /* Serious Error: Block here for debugging */
        (void)xTask;
        (void)pcTaskName;

        // Blink LED fast to indicate error
        while (1)
        {
            BSP_LED_Toggle(LED2);
            // Fast delay loop (approx 100ms)
            for (volatile int i = 0; i < 100000; i++)
            {

            }
        }
    }

    /* Hook called if pvPortMalloc fails (Optional but recommended) */
    // cppcheck-suppress unusedFunction
    void vApplicationMallocFailedHook(void)
    {
        /* Serious Error: Heap is full */
        while (1)
        {

        }
    }

    /* ============================ TASK HOOKS END ============================== */

    #ifdef USE_FULL_ASSERT

    /**
     * @brief  Reports the name of the source file and the source line number
     *         where the assert_param error has occurred.
     * @param  file: pointer to the source file name
     * @param  line: assert_param error line source number
     * @retval None
     */
    void assert_failed(uint8_t *file, uint32_t line)
    {
        /* User can add his own implementation to report the file name and line
        number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
        file, line) */
        (void)file;
        (void) line;
        /* Infinite loop */
        while (1)
        {
        }
    }
    #endif

    /**
     * @}
     */

    /**
     * @}
     */

    /**************************************************************************** */
}

/* ============================== INTERRUPT SERVICE ROUTINES ====================================== */
extern "C" {

    /* UART Rx Complete Callback */
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
    {
        if (huart != NULL && huart->Instance == USART1) 
        {
            if (FreeRTOS_Cpp::appLogger::instance != nullptr)
            {
                // Grab the character that was just received
                uint8_t rxChar = *(FreeRTOS_Cpp::appLogger::instance->getRxBuffer());
                
                // Pass it to the abstracted logger
                FreeRTOS_Cpp::appLogger::instance->notifyCommandReceivedFromISR(rxChar);

                // Re-enable the hardware interrupt for the next character
                (void)HAL_UART_Receive_IT(huart, const_cast<uint8_t*>(FreeRTOS_Cpp::appLogger::instance->getRxBuffer()), 1u);
            }
        }
    }

    /* QSPI Status Match Callback */
    void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        if (FreeRTOS_Cpp::appLogger::_eraseCompleteMutex != nullptr)
        {
            (void)xSemaphoreGiveFromISR(
                static_cast<SemaphoreHandle_t>(FreeRTOS_Cpp::appLogger::_eraseCompleteMutex), 
                &xHigherPriorityTaskWoken
            );
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    
    /* Hardware-level Pin Multiplexing for UART */
    // cppcheck-suppress constParameterPointer
    void HAL_UART_MspInit(UART_HandleTypeDef* huart)
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        if(huart->Instance == USART1)
        {
            /* 1. Enable Peripheral Clocks */
            __HAL_RCC_USART1_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
            
            /* 2. Configure PB6 (TX) and PB7 (RX) for USART1 Alternate Function */
            GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
            
            /* 3. Enable the Interrupt in the NVIC for FreeRTOS */
            HAL_NVIC_SetPriority(USART1_IRQn, 5, 0); // Priority 5 is safe for FreeRTOS API calls
            HAL_NVIC_EnableIRQ(USART1_IRQn);
        }
    }

    TIM_HandleTypeDef htimHalTick; // Handle for the new HAL timebase

    /**
     * @brief Override HAL_InitTick to use TIM7 instead of SysTick.
     * Called automatically by HAL_Init() and HAL_RCC_ClockConfig().
     */
    HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
    {
        RCC_ClkInitTypeDef clkconfig;
        uint32_t uwTimclock, uwAPB1Prescaler;
        uint32_t uwTickPrios = TickPriority;
        HAL_StatusTypeDef status;

        /* 1. Enable TIM7 Clock */
        __HAL_RCC_TIM7_CLK_ENABLE();

        /* 2. Get Clock Frequencies */
        HAL_RCC_GetClockConfig(&clkconfig, &uwAPB1Prescaler);
        uwTimclock = HAL_RCC_GetPCLK1Freq();

        /* Compute TIM7 clock; APB1 prescaler usually multiplies clock for timers */
        if (uwAPB1Prescaler != RCC_HCLK_DIV1) {
            uwTimclock = 2U * uwTimclock;
        }

        /* 3. Configure Timer for 1ms overflows */
        htimHalTick.Instance = TIM7;
        htimHalTick.Init.Prescaler = (uint32_t)((uwTimclock / 1000000U) - 1U); // 1MHz clock
        htimHalTick.Init.Period = (1000U - 1U);                               // 1kHz overflow (1ms)
        htimHalTick.Init.CounterMode = TIM_COUNTERMODE_UP;
        htimHalTick.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

        status = HAL_TIM_Base_Init(&htimHalTick);
        if (status != HAL_OK) return status;

        /* 4. Configure NVIC and Start Interrupt */
        HAL_NVIC_SetPriority(TIM7_IRQn, uwTickPrios, 0);
        HAL_NVIC_EnableIRQ(TIM7_IRQn);

        return HAL_TIM_Base_Start_IT(&htimHalTick);
    }

    /**
     * @brief This is the callback triggered by the HAL_TIM_IRQHandler above.
     */
    // cppcheck-suppress constParameterPointer
    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
    {
        if (htim->Instance == TIM7) {
            HAL_IncTick(); // The HAL heartbeat now lives here!
        }
    }

    /**
     * @brief Must also override Suspend/Resume for HAL power management
     */
    void HAL_SuspendTick(void) 
    {
        __HAL_TIM_DISABLE_IT(&htimHalTick, TIM_IT_UPDATE); 
    }

    void HAL_ResumeTick(void)  
    {
        __HAL_TIM_ENABLE_IT(&htimHalTick, TIM_IT_UPDATE); 
    }

    /* ============================== HARDWARE MSP OVERRIDES ====================================== */
    /**
     * @brief I2C MSP Initialization
     * This function configures the hardware resources used in this example:
     * - Peripheral's clock enable
     * - Peripheral's GPIO Configuration
     * @param hi2c: I2C handle pointer
     * @retval None
     */
    // cppcheck-suppress constParameterPointer
    void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        /* The sensors on the B-L475E-IOT01A board are connected to I2C2 */
        if(hi2c->Instance == I2C2)
        {
            /* 1. Enable Peripheral Clocks */
            __HAL_RCC_GPIOB_CLK_ENABLE();
            __HAL_RCC_I2C2_CLK_ENABLE();

            /* 2. Configure I2C2 GPIO pins
                PB10     ------> I2C2_SCL
                PB11     ------> I2C2_SDA
            */
            GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_11;
            GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;         // Open Drain is mandatory for I2C
            GPIO_InitStruct.Pull      = GPIO_PULLUP;             // Enable internal pull-ups
            GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;           // AF4 connects these pins to the I2C2 hardware

            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

            /* 3. Force a bus reset (Optional but highly recommended) 
                This clears the "Busy" state if the MCU was reset mid-transaction. */
            __HAL_RCC_I2C2_FORCE_RESET();
            __HAL_RCC_I2C2_RELEASE_RESET();
        }
    }

    /**
     * @brief I2C MSP De-Initialization
     * This function frees the hardware resources used in this example:
     * - Disable the Peripheral's clock
     * - Revert GPIO to their default state
     * @param hi2c: I2C handle pointer
     * @retval None
     */
    // cppcheck-suppress constParameterPointer
    void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
    {
        if(hi2c->Instance == I2C2)
        {
            /* 1. Disable Peripheral Clock */
            __HAL_RCC_I2C2_CLK_DISABLE();

            /* 2. De-Initialize I2C2 GPIO pins */
            HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_11);
        }
    }
}

//Main is not in scope of namespace FreeRTOS_App
/* ============================== MAIN ====================================== */
int main(void)
{
    /* STM32L4xx HAL library initialization:
    - Configure the Flash prefetch, Flash preread and Buffer caches
    - Systick timer is configured by default as source of time base, but user
        can eventually implement his proper time base source (a general
    purpose timer for example or other time source), keeping in mind that Time
    base duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
        handled in milliseconds basis.
    - Low Level Initialization
    */
    (void)HAL_Init();

    /* Configure the System clock to have a frequency of 80 MHz */
    FreeRTOS_Cpp::SystemClock_Config();

    // FreeRTOS_Cpp::Force_I2C2_Clock_On();
    FreeRTOS_Cpp::UART1_Init();
    
    // Initialize the Green LED (LED2)
    BSP_LED_Init(LED2);
    
   /* Create Logger RTOS Primitives */
    QueueHandle_t hPrintQueue = xQueueCreateStatic(FreeRTOS_Cpp::appLogger::QUEUE_LEN, LOG_MESSAGE_SIZE,
                                                    FreeRTOS_Cpp::ucPrintQueueStorage, &FreeRTOS_Cpp::xPrintQueueBuffer);
    QueueHandle_t hEventQueue = xQueueCreateStatic(FreeRTOS_Cpp::appLogger::QUEUE_LEN, LOG_ENTRY_SIZE, 
                                                    FreeRTOS_Cpp::ucEventQueueStorage, &FreeRTOS_Cpp::xEventQueueBuffer);
    QueueHandle_t hCommandQueue = xQueueCreateStatic(10u, COMMAND_ENTRY_SIZE, 
                                                    FreeRTOS_Cpp::ucCmmandQueueStorage, &FreeRTOS_Cpp::xCommandQueueBuffer);
    
    SemaphoreHandle_t hUartMutex = xSemaphoreCreateMutexStatic(&FreeRTOS_Cpp::xUartMutexBuffer);
    SemaphoreHandle_t hQspiMutex = xSemaphoreCreateMutexStatic(&FreeRTOS_Cpp::xQspiMutexBuffer);
    SemaphoreHandle_t hEraseCompleteMutex = xSemaphoreCreateBinaryStatic(&FreeRTOS_Cpp::xEraseCompleteMutexBuffer);

    /* Initialize RTOS Primitives statically first */
    FreeRTOS_Cpp::xSystemEventGroup = xEventGroupCreateStatic(&FreeRTOS_Cpp::xSystemEventGroupBuffer);
    FreeRTOS_Cpp::xWatchdogEventGroup = xEventGroupCreateStatic(&FreeRTOS_Cpp::xWatchdogEventGroupBuffer);

    /* Object Instantiation Declared 'static' so they persist in memory safely out of the main stack frame. */
    static FreeRTOS_Cpp::appLogger     myLogger(&FreeRTOS_Cpp::realRtos, &FreeRTOS_Cpp::realHw, 
                                                FreeRTOS_Cpp::xSystemEventGroup, FreeRTOS_Cpp::xWatchdogEventGroup, 
                                                &FreeRTOS_Cpp::IWDG_handle);

    static FreeRTOS_Cpp::AppHeartbeat  myHeartbeat(&FreeRTOS_Cpp::realRtos,  &FreeRTOS_Cpp::realHw,
                                                FreeRTOS_Cpp::xSystemEventGroup, FreeRTOS_Cpp::xWatchdogEventGroup);

    static FreeRTOS_Cpp::appSensorRead mySensor(&FreeRTOS_Cpp::realRtos, &FreeRTOS_Cpp::realTempSensor, &FreeRTOS_Cpp::realHumiditySensor, 
                                                FreeRTOS_Cpp::xSystemEventGroup, FreeRTOS_Cpp::xWatchdogEventGroup);

    static FreeRTOS_Cpp::systemManager mySysManager(&FreeRTOS_Cpp::realRtos,  &FreeRTOS_Cpp::realHw, &mySensor, 
                                                FreeRTOS_Cpp::xSystemEventGroup, FreeRTOS_Cpp::xWatchdogEventGroup,
                                                &FreeRTOS_Cpp::IWDG_handle);
    
    /* Static Task Allocation Buffers */
    static StaticTask_t xHeartbeatTaskTCB;
    static StackType_t  xheartbeatTaskStack[FreeRTOS_Cpp::TASK_STACK_SIZE_HEARTBEAT_TASK];

    static StaticTask_t xSysMgrTaskTCB;
    static StackType_t  xSysMgrTaskStack[FreeRTOS_Cpp::TASK_STACK_SIZE_SYS_MANAGER_TASK];
    
    static StaticTask_t xvSensorReadTaskTCB;
    static StackType_t  xSensorReadStack[FreeRTOS_Cpp::TASK_STACK_SIZE_SENSOR_READ_TASK];

    static StaticTask_t xLoggerTaskTCB;
    static StackType_t  xLoggerTaskStack[FreeRTOS_Cpp::TASK_STACK_SIZE_APPLOGGER_TASK];

    static StaticTask_t xCommandTaskTCB;
    static StackType_t  xCommandTaskStack[FreeRTOS_Cpp::TASK_STACK_SIZE_COMMAND_TASK];

    myLogger.init(hPrintQueue, hEventQueue, hCommandQueue, hUartMutex, hQspiMutex, hEraseCompleteMutex);

    FreeRTOS_Cpp::xHeartBeatTaskHandle = xTaskCreateStatic(FreeRTOS_Cpp::AppHeartbeat::HeartBeatTask, 
                    "Heartbeat", 
                    FreeRTOS_Cpp::TASK_STACK_SIZE_HEARTBEAT_TASK, 
                    &myHeartbeat,                     // Pass object instance pointer
                    FreeRTOS_Cpp::TASK_PRIORITY_HEARTBEAT_TASK, 
                    xheartbeatTaskStack, &xHeartbeatTaskTCB);

    FreeRTOS_Cpp::xvSensorReadTaskHandle = xTaskCreateStatic(FreeRTOS_Cpp::appSensorRead::vSensorReadTask, 
                    "SensorRead", 
                    FreeRTOS_Cpp::TASK_STACK_SIZE_SENSOR_READ_TASK, 
                    &mySensor,                        // Pass object instance pointer
                    FreeRTOS_Cpp::TASK_PRIORITY_SENSOR_READ_TASK, 
                    xSensorReadStack, &xvSensorReadTaskTCB);

    FreeRTOS_Cpp::xsystemManagerTaskHandle = xTaskCreateStatic(FreeRTOS_Cpp::systemManager::systemManagerTask, 
                    "SysManager", 
                    FreeRTOS_Cpp::TASK_STACK_SIZE_SYS_MANAGER_TASK, 
                    &mySysManager,                    // Pass object instance pointer
                    FreeRTOS_Cpp::TASK_PRIORITY_SYS_MANAGER_TASK, 
                    xSysMgrTaskStack, &xSysMgrTaskTCB);

    FreeRTOS_Cpp::xAppLoggerTaskHandle = xTaskCreateStatic(FreeRTOS_Cpp::appLogger::vAppLoggerTask, 
                    "Logger", 
                    FreeRTOS_Cpp::TASK_STACK_SIZE_APPLOGGER_TASK, 
                    &myLogger,                        // Pass object instance pointer
                    FreeRTOS_Cpp::TASK_PRIORITY_APPLOGGER_TASK, 
                    xLoggerTaskStack, &xLoggerTaskTCB);

    FreeRTOS_Cpp::xAppCommandTaskHandle = xTaskCreateStatic(FreeRTOS_Cpp::appLogger::vCommandTask, 
                    "Command", 
                    FreeRTOS_Cpp::TASK_STACK_SIZE_COMMAND_TASK, 
                    &myLogger,                        // Pass object instance pointer
                    FreeRTOS_Cpp::TASK_PRIORITY_COMMAND_TASK, 
                    xCommandTaskStack, &xCommandTaskTCB);

    // Start Scheduler (Should not return)
    vTaskStartScheduler();

    // Loop forever if scheduler fails
    while (1)
    {

    }
}