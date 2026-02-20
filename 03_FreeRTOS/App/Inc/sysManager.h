#ifndef SYS_MANAGER_H
#define SYS_MANAGER_H


#define SENSOR_READ_SLEEP_DURATION  1000U
#define SYS_MANAGER_SLEEP_DURATION  5000U


typedef enum {
    SYS_STATE_INIT_HARDWARE,  /* Initializing BSP */
    SYS_STATE_OPERATIONAL,    /* System is healthy and reading data */
    SYS_STATE_FAULT           /* A component failed; entering safe mode */
} SystemState_t;

extern SystemState_t currentState;

void vSystemManagerTask(void *pvParameters) ;
#endif //SYS_MANAGER_H