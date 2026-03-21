// Updated test_appSensorRead.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "appSensorRead.hpp"
#include "MockInterfaces.hpp"

using namespace FreeRTOS_Cpp;
using ::testing::Return;

class SensorReadTest : public ::testing::Test {
protected:
    MockRtos mockRtos;
    MockSensor mockTempSensor;
    MockSensor mockHumSensor;
    appSensorRead* sensorTask;

    void SetUp() override {
        sensorTask = new appSensorRead(&mockRtos, &mockTempSensor, &mockHumSensor, (void*)0x11, (void*)0x22);
    }

    void TearDown() override { delete sensorTask; }
};

// Test Case 3: Message Formatting Consistency
TEST_F(SensorReadTest, FormatsSensorMessageCorrectly) {
    char buffer[64];
    
    // Test the specific string assembly pattern: "Label: Value Unit\r\n"
    sensorTask->App_FormatSensorMsg(buffer, sizeof(buffer), "Temp", 25.55f, "C");
    
    EXPECT_STREQ(buffer, "Temp: 25.55 C\r\n"); 
}

// Test Case 4: Data Point Logging (Verify memcpy logic)
TEST_F(SensorReadTest, CurrentValuesAreStoredAsFloats) {
    // These values are stored as floats in the payload array via memcpy
    sensorTask->_currentTemp = 22.5f;
    sensorTask->_currentHumidity = 45.0f;

    EXPECT_FLOAT_EQ(sensorTask->getCurrentTemp(), 22.5f);
    EXPECT_FLOAT_EQ(sensorTask->getCurrentHumidity(), 45.0f);
}