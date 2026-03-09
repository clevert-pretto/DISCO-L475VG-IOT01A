// standard includes
#include <string.h>

// hardware includes
#include "main.hpp"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"

//Application includes
#include "appHeartbeat.hpp"
#include "appSensorRead.hpp"
#include "appLogger.hpp"
#include "sysManager.hpp"


/*===================== FreeRTOS Hook Prototypes ============================ */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, 
                                    StackType_t **ppxIdleTaskStackBuffer, 
                                    size_t  *pulIdleTaskStackSize);

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
                                    StackType_t **ppxTimerTaskStackBuffer, 
                                    size_t  *pulTimerTaskStackSize);

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void vApplicationMallocFailedHook(void);
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
    SystemClock_Config();

    // Initialize the Green LED (LED2)
    BSP_LED_Init(LED2);
    
    /* Initialize RTOS Primitives statically first */
    xSystemEventGroup = xEventGroupCreateStatic(&xSystemEventGroupBuffer);
    xWatchdogEventGroup = xEventGroupCreateStatic(&xWatchdogEventGroupBuffer);

    /* Object Instantiation Declared 'static' so they persist in memory safely out of the main stack frame. */
    static appLogger     myLogger(&discoveryUART1, &QSPIHandle, xSystemEventGroup, xWatchdogEventGroup);
    static AppHeartbeat  myHeartbeat(xSystemEventGroup, xWatchdogEventGroup);
    static appSensorRead mySensor(xSystemEventGroup, xWatchdogEventGroup);
    static systemManager mySysManager;
    
    /* Static Task Allocation Buffers */
    static StaticTask_t xHeartbeatTaskTCB;
    static StackType_t  xheartbeatTaskStack[TASK_STACK_SIZE_HEARTBEAT_TASK];

    static StaticTask_t xSysMgrTaskTCB;
    static StackType_t  xSysMgrTaskStack[TASK_STACK_SIZE_SYS_MANAGER_TASK];
    
    static StaticTask_t xvSensorReadTaskTCB;
    static StackType_t  xSensorReadStack[TASK_STACK_SIZE_SENSOR_READ_TASK];

    static StaticTask_t xLoggerTaskTCB;
    static StackType_t  xLoggerTaskStack[TASK_STACK_SIZE_APPLOGGER_TASK];

    static StaticTask_t xCommandTaskTCB;
    static StackType_t  xCommandTaskStack[TASK_STACK_SIZE_COMMAND_TASK];

    myLogger.init();

    xHeartBeatTaskHandle = xTaskCreateStatic(AppHeartbeat::HeartBeatTask, 
                      "Heartbeat", 
                      TASK_STACK_SIZE_HEARTBEAT_TASK, 
                      &myHeartbeat,                     // Pass object instance pointer
                      TASK_PRIORITY_HEARTBEAT_TASK, 
                      xheartbeatTaskStack, &xHeartbeatTaskTCB);

    xsystemManagerTaskHandle = xTaskCreateStatic(systemManager::systemManagerTask, 
                      "SysManager", 
                      TASK_STACK_SIZE_SYS_MANAGER_TASK, 
                      &mySysManager,                    // Pass object instance pointer
                      TASK_PRIORITY_SYS_MANAGER_TASK, 
                      xSysMgrTaskStack, &xSysMgrTaskTCB);

    xvSensorReadTaskHandle = xTaskCreateStatic(appSensorRead::vSensorReadTask, 
                      "SensorRead", 
                      TASK_STACK_SIZE_SENSOR_READ_TASK, 
                      &mySensor,                        // Pass object instance pointer
                      TASK_PRIORITY_SENSOR_READ_TASK, 
                      xSensorReadStack, &xvSensorReadTaskTCB);

    xAppLoggerTaskHandle = xTaskCreateStatic(appLogger::vAppLoggerTask, 
                      "Logger", 
                      TASK_STACK_SIZE_APPLOGGER_TASK, 
                      &myLogger,                        // Pass object instance pointer
                      TASK_PRIORITY_APPLOGGER_TASK, 
                      xLoggerTaskStack, &xLoggerTaskTCB);

    xAppCommandTaskHandle = xTaskCreateStatic(appLogger::vCommandTask, 
                      "Command", 
                      TASK_STACK_SIZE_COMMAND_TASK, 
                      &myLogger,                        // Pass object instance pointer
                      TASK_PRIORITY_COMMAND_TASK, 
                      xCommandTaskStack, &xCommandTaskTCB);

    // Start Scheduler (Should not return)
    vTaskStartScheduler();

    // Loop forever if scheduler fails
    while (1)
    {

    }
}



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