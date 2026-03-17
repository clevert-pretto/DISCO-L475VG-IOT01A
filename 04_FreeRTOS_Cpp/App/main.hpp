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
#ifndef MAIN_HPP
#define MAIN_HPP

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"

// Kernel includes
#include "FreeRTOS.h"
#include "event_groups.h"

namespace FreeRTOS_Cpp
{
  /* Exported types ------------------------------------------------------------*/
  /* Exported constants --------------------------------------------------------*/
  /* External declaration for MISRA Rule 8.4 */
  #ifdef __cplusplus
  extern "C"{
  #endif
  extern UART_HandleTypeDef discoveryUART1;
  extern QSPI_HandleTypeDef QSPIHandle;
  extern IWDG_HandleTypeDef IWDG_handle;
  #ifdef __cplusplus
  }
  #endif
  /* Handle for the Event Group */
  extern EventGroupHandle_t xSystemEventGroup;
  extern EventGroupHandle_t xWatchdogEventGroup;

  /* Task Handles for direct monitoring */
  extern TaskHandle_t xvSensorReadTaskHandle;     //For Sensor read task
  extern TaskHandle_t xsystemManagerTaskHandle;   //For System manager task
  extern TaskHandle_t xHeartBeatTaskHandle;       //For heart beat task
  extern TaskHandle_t xAppLoggerTaskHandle;       //For App Logger task
  extern TaskHandle_t xAppCommandTaskHandle;      //For Command task

  /* Exported macro ------------------------------------------------------------*/
 


  /* Exported functions ------------------------------------------------------- */
}
#endif /* MAIN_HPP */

