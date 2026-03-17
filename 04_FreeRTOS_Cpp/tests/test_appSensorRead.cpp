#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "appSensorRead.hpp"
#include "appDefines.hpp"

/* Include the Shared Mocks */
#include "MockInterfaces.hpp"


using namespace FreeRTOS_Cpp;
using ::testing::Return;


class SensorReadTest : public ::testing::Test {
protected:
    MockRtos mockRtos;
    MockSensor mockTempSensor;
    MockSensor mockHumSensor;
    void* dummySysEvents = reinterpret_cast<void*>(0x11);
    void* dummyWdgEvents = reinterpret_cast<void*>(0x22);

    appSensorRead* sensorTask;

    void SetUp() override {
        sensorTask = new appSensorRead(&mockRtos, &mockTempSensor, &mockHumSensor, dummySysEvents, dummyWdgEvents);
    }

    void TearDown() override {
        delete sensorTask;
    }
};

// Test Case 1: Successful Initialization
TEST_F(SensorReadTest, InitSuccessSetsBothBits) {
    EXPECT_CALL(mockTempSensor, init()).WillOnce(Return(true));
    EXPECT_CALL(mockHumSensor, init()).WillOnce(Return(true));

    uint8_t result = sensorTask->appSensorRead_Init();
    
    // Both sensor bits should be set (appSENSOR_TEMPERATURE | appSENSOR_HUMIDITY)
    EXPECT_EQ(result, 3U); 
}

// Test Case 2: Partial Initialization Failure
TEST_F(SensorReadTest, InitFailsTemperatureOnly) {
    EXPECT_CALL(mockTempSensor, init()).WillOnce(Return(false)); // Temp fails
    EXPECT_CALL(mockHumSensor, init()).WillOnce(Return(true));   // Humidity succeeds

    uint8_t result = sensorTask->appSensorRead_Init();
    
    EXPECT_EQ(result, 2U); // Only appSENSOR_HUMIDITY (2U) should be set
}

// Test Case 3: Message Formatting
TEST_F(SensorReadTest, FormatsSensorMessageCorrectly) {
    char buffer[64];
    
    sensorTask->App_FormatSensorMsg(buffer, sizeof(buffer), "Temp", 25.5f, "C");
    
    // Expects the xtoa formatting to be applied correctly
    EXPECT_STREQ(buffer, "Temp: 25.50 C\r\n"); 
}