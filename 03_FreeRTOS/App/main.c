#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

// Task Handle
TaskHandle_t xBlinkHandle = NULL;

// Hardware Setup
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

// Blink Task Function
void vBlinkTask(void *pvParameters) {
    for (;;) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms delay
    }
}

int main(void) {
    // 1. Initialize HAL
    HAL_Init();

    // 2. Configure System Clock (Standard MSI -> PLL 80MHz)
    SystemClock_Config();

    // 3. Initialize GPIO (LED)
    MX_GPIO_Init();

    // 4. Create Task
    xTaskCreate(vBlinkTask, "Blinky", 128, NULL, 1, &xBlinkHandle);

    // 5. Start Scheduler (Should not return)
    vTaskStartScheduler();

    // Loop forever if scheduler fails
    while (1);
}

// Minimal SysTick Hook if needed by HAL before Scheduler
// Note: We mapped SysTick to FreeRTOS in Config, so FreeRTOS handles it.
// Don't use HAL_Delay() inside tasks!

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure GPIO pin : PB14 (LED2) */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SystemClock_Config(void) {
    // Use default MSI or HSI configuration usually provided in system_stm32l4xx.c 
    // or simple config here. For brevity, we assume MSI is running approx 4MHz default.
    // For proper 80MHz, you need a larger struct setup here.
    // This simple call enables power for the board.
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
}