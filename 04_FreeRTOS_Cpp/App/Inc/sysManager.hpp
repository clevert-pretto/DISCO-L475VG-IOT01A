#ifndef SYS_MANAGER_HPP
#define SYS_MANAGER_HPP

typedef enum {
    SYS_STATE_INIT_HARDWARE,  /* Initializing BSP */
    SYS_STATE_OPERATIONAL,    /* System is healthy and reading data */
    SYS_STATE_FAULT           /* A component failed; entering safe mode */
} SystemState_t;

class systemManager {
    
    public:
        systemManager();
        static void systemManagerTask(void *pvParameters);

        static constexpr uint32_t  appSensorReadSleepDuration = 1000U;

    private:
        SystemState_t currentState;
        void handleHardwareInit(void);
        void reportInitFailure(uint8_t sensorStatus, uint8_t qspiStatus);
};


#endif //SYS_MANAGER_HPP