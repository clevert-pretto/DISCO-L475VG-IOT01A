// standard includes
#include <string.h>

// hardware includes
#include "main.h"

// Kernel includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"

//Application includes
#include "appHeartbeat.h"
#include "appSensorRead.h"
#include "appLogger.h"
#include "sysManager.h"

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
UART_HandleTypeDef discoveryUART1;
EventGroupHandle_t xSystemEventGroup;

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
static void BSP_UART1_Init(uint32_t BaudRate)
{
    /* Configure the UART parameters BEFORE calling BSP_COM_Init */
    discoveryUART1.Instance = USART1; // Optional, BSP_COM_Init sets this too
    discoveryUART1.Init.BaudRate               = BaudRate;
    discoveryUART1.Init.WordLength             = UART_WORDLENGTH_8B;
    discoveryUART1.Init.StopBits               = UART_STOPBITS_1;
    discoveryUART1.Init.Parity                 = UART_PARITY_NONE;
    discoveryUART1.Init.Mode                   = UART_MODE_TX_RX;
    discoveryUART1.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    discoveryUART1.Init.OverSampling           = UART_OVERSAMPLING_16;
    discoveryUART1.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    discoveryUART1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    BSP_COM_Init(COM1, &discoveryUART1);
}

/* ============================== MAIN ====================================== */
int main(void)
{
    //For Heartbeat task
    static StaticTask_t xHeartbeatTaskTCB;
    static StackType_t  xheartbeatTaskStack[TASK_STACK_SIZE_HEARTBEAT_TASK];
    //For system manager task
    static StaticTask_t xSysMgrTaskTCB;
    static StackType_t  xSysMgrTaskStack[TASK_STACK_SIZE_SYS_MANAGER_TASK];
    
    //For sensor read task
    static StaticTask_t xvSensorReadTaskTCB;
    static StackType_t  xSensorReadStack[TASK_STACK_SIZE_SENSOR_READ_TASK];
    
    ////For App Logger task
    static TaskHandle_t xAppLoggerTaskHandle = NULL;

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

    // Initialize UART as Virtual COM Port
    BSP_UART1_Init(DISCO_BOARD_VCP_BAUDRATE);

    appLogger_Init();

    /* Create the event group */
    xSystemEventGroup = xEventGroupCreate();

    if (xSystemEventGroup == NULL) {
        // Handle error: No memory for Event Group
    }
    
    /* Create a HeartBeat Task (Permanent, deterministic memory) */
    (void)xTaskCreateStatic(HeartBeatTask,              // Function
                      "HeartBeatTask",                  // Name
                      TASK_STACK_SIZE_HEARTBEAT_TASK,   // Stack Size
                      NULL,                             // Parameter
                      TASK_PRIORITY_HEARTBEAT_TASK,     // Priority
                      xheartbeatTaskStack,              // Stack Buffer
                      &xHeartbeatTaskTCB                // TCB Buffer
    );

    /* Create a system state machine manager Task (Permanent, deterministic memory) */
    (void)xTaskCreateStatic(vSystemManagerTask,         // Function
                      "SysManagerTask",                 // Name
                      TASK_STACK_SIZE_SYS_MANAGER_TASK, // Stack Size
                      NULL,                             // Parameter (heart beat Blink)
                      TASK_PRIORITY_SYS_MANAGER_TASK,   // Priority
                      xSysMgrTaskStack,                 // Stack Buffer
                      &xSysMgrTaskTCB                   // TCB Buffer
    );
    /* Create Sensor Read task (Permenant, deterministic memory)*/
    (void)xTaskCreateStatic(vSensorReadTask,            // Function
                      "SensorReadTask",                 // Name
                      TASK_STACK_SIZE_SENSOR_READ_TASK, // Stack Size
                      (void *)NULL,                     // Parameter
                      TASK_PRIORITY_SENSOR_READ_TASK,   // Priority
                      xSensorReadStack,                 // Stack Buffer
                      &xvSensorReadTaskTCB              // TCB Buffer
    );

    /* Create a DYNAMIC Task (Flexible, uses Heap) */
    (void)xTaskCreate(vAppLoggerTask,                   // Function
                "AppLoggerTask",                        // Name
                TASK_STACK_SIZE_APPLOGGER_TASK,         // Stack Size
                (void *)NULL,                           // Parameter
                TASK_PRIORITY_APPLOGGER_TASK,           // Priority
                &xAppLoggerTaskHandle                   // Handle Storage
    );

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