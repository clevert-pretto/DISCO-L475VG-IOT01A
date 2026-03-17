#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sysManager.hpp"
#include "appLogger.hpp"
#include "appDefines.hpp"
/* Include the Shared Mocks */
#include "MockInterfaces.hpp"

using namespace FreeRTOS_Cpp;
using ::testing::Return;
using ::testing::_;


class SysManagerTest : public ::testing::Test {
protected:
    MockRtos mockRtos;
    MockHardware mockHw;
    MockSensor mockTempSensor;
    MockSensor mockHumSensor;
    
    appSensorRead* sensorTask;
    appLogger* logger;
    systemManager* sysMgr;

    void SetUp() override {
        sensorTask = new appSensorRead(&mockRtos, &mockTempSensor, &mockHumSensor, nullptr, nullptr);
        
        // Instantiate logger to satisfy the appLogger::instance singleton requirement
        logger = new appLogger(&mockRtos, &mockHw, nullptr, nullptr, nullptr);
        logger->init(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

        sysMgr = new systemManager(&mockRtos, &mockHw, sensorTask, nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        delete sysMgr;
        delete logger;
        delete sensorTask;
    }
};

// Test Case 1: Hardware Init Success State Transition
TEST_F(SysManagerTest, HardwareInitSuccessTransitionsToOperational) {
    // 1. Watchdog Init Succeeds
    EXPECT_CALL(mockHw, watchdog_Init(_)).WillOnce(Return(0)); /* HAL_OK */
    EXPECT_CALL(mockRtos, delay(200)).Times(1);
    EXPECT_CALL(mockHw, watchdog_refresh(_)).Times(1);

    // 2. Sensors Init Succeeds
    EXPECT_CALL(mockTempSensor, init()).WillOnce(Return(true));
    EXPECT_CALL(mockHumSensor, init()).WillOnce(Return(true));

    // 3. QSPI Storage Init Succeeds
    EXPECT_CALL(mockHw, storageInit(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mockHw, storageRead(_, _, _)).WillOnce(Return(false)); // For header scan

    // 4. Expect Event Bits to be set to SUCCESS
    EXPECT_CALL(mockRtos, clearEventBits(_, _)).Times(1);
    EXPECT_CALL(mockRtos, setEventBits(_, EVENT_BIT_INIT_SUCCESS)).Times(1);

    sysMgr->handleHardwareInit();

    EXPECT_EQ(sysMgr->currentState, SYS_STATE_OPERATIONAL);
}

// Test Case 2: Hardware Init Failure (Sensor Failure)
TEST_F(SysManagerTest, HardwareInitFailureTransitionsToFault) {
    EXPECT_CALL(mockHw, watchdog_Init(_)).WillOnce(Return(0));
    EXPECT_CALL(mockRtos, delay(200)).Times(1);
    EXPECT_CALL(mockHw, watchdog_refresh(_)).Times(1);

    // Force Sensor Failure
    EXPECT_CALL(mockTempSensor, init()).WillOnce(Return(false));
    EXPECT_CALL(mockHumSensor, init()).WillOnce(Return(true));

    EXPECT_CALL(mockHw, storageInit(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mockHw, storageRead(_, _, _)).WillOnce(Return(false));

    // Expect Event Bits to be set to FAILED
    EXPECT_CALL(mockRtos, setEventBits(_, EVENT_BIT_INIT_FAILED)).Times(1);

    sysMgr->handleHardwareInit();

    EXPECT_EQ(sysMgr->currentState, SYS_STATE_FAULT);
}