/**
  ******************************************************************************
  * @file    Templates/Inc/main.h 
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MAIN_H
#define MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"

// Kernel includes
#include "FreeRTOS.h"
#include "event_groups.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External declaration for MISRA Rule 8.4 */
extern UART_HandleTypeDef discoveryUART1;

/* Exported macro ------------------------------------------------------------*/
#define TASK_PRIORITY_HEARTBEAT_TASK      0
#define TASK_PRIORITY_SYS_MANAGER_TASK    3
#define TASK_PRIORITY_SENSOR_READ_TASK    1
#define TASK_PRIORITY_APPLOGGER_TASK      2
#define TASK_PRIORITY_COMMAND_TASK        2

#define TASK_STACK_SIZE_HEARTBEAT_TASK    configMINIMAL_STACK_SIZE
#define TASK_STACK_SIZE_SYS_MANAGER_TASK  (configMINIMAL_STACK_SIZE * 2u)
#define TASK_STACK_SIZE_SENSOR_READ_TASK  (configMINIMAL_STACK_SIZE * 4u)
#define TASK_STACK_SIZE_APPLOGGER_TASK    (configMINIMAL_STACK_SIZE * 4u)
#define TASK_STACK_SIZE_COMMAND_TASK      (configMINIMAL_STACK_SIZE * 4u)

#define DISCO_BOARD_VCP_BAUDRATE          (uint32_t) 115200
#define DISCO_BOARD_UART_TIMEOUT_MS       100U

/* Event Group Bits */
#define EVENT_BIT_INIT_SUCCESS    (1UL << 0U)
#define EVENT_BIT_INIT_FAILED     (1UL << 1U)
#define EVENT_BIT_FAULT_DETECTED  (1UL << 2U)


#define TASK_ID_SYS_MANAGER     1U
#define TASK_ID_HEART_BEAT      2U
#define TASK_ID_SENSOR_READ     3U
#define TASK_ID_APP_LOGGER      4U


/* Handle for the Event Group */
extern EventGroupHandle_t xSystemEventGroup;
  extern TaskHandle_t xAppLoggerTaskHandle;
  extern TaskHandle_t xAppCommandTaskHandle;
/* Exported functions ------------------------------------------------------- */

#endif /* MAIN_H */

