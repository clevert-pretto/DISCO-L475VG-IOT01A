// Updated test_appLogger.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "appLogger.hpp"
#include "MockInterfaces.hpp"

using namespace FreeRTOS_Cpp;
using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;

class AppLoggerTest : public ::testing::Test {
protected:
    MockRtos mockRtos;
    MockHardware mockHw;
    appLogger* logger;

    void SetUp() override {
        logger = new appLogger(&mockRtos, &mockHw, nullptr, nullptr, nullptr, nullptr);
        logger->init((void*)1, (void*)2, (void*)3, (void*)4, (void*)5, (void*)6);
    }

    void TearDown() override { delete logger; }
};

// Test Case 1: Updated Command Logic (String-based)
TEST_F(AppLoggerTest, HandlesDumpLogsStringCommand) {
    logger->_u32CurrentWriteAddress = LOG_DATA_START + LOG_ENTRY_SIZE;

    EXPECT_CALL(mockRtos, takeMutex(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockRtos, giveMutex(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockHw, storageRead(_, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockHw, printLog(_, _)).Times(testing::AtLeast(1));

    // Refactored: Use string command instead of 'd'
    logger->handleCommand("dump_logs"); 
}

// Test Case 2: Task Registry Stack Monitoring
TEST_F(AppLoggerTest, CheckStackUsageIteratesThroughRegistry) {
    const char* dummyName = "TestTask";
    void* dummyHandle = (void*)0xDEADBEEF;

    // Set up expectations for the registry loop
    EXPECT_CALL(mockRtos, getRegisteredTaskCount()).WillOnce(Return(1));
    EXPECT_CALL(mockRtos, getRegisteredTaskInfo(0, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(dummyName), SetArgPointee<2>(dummyHandle), Return(true)));
    
    EXPECT_CALL(mockRtos, getStackHighWaterMark(dummyHandle)).WillOnce(Return(128));
    EXPECT_CALL(mockRtos, takeMutex(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockRtos, giveMutex(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockHw, printLog(_, _)).Times(testing::AtLeast(1));

    logger->handleCommand("stack_health");
}

TEST_F(AppLoggerTest, RejectsInvalidStringCommand) {
    EXPECT_CALL(mockRtos, takeMutex(_, _)).WillOnce(Return(true));
    EXPECT_CALL(mockRtos, giveMutex(_)).WillOnce(Return(true));
    EXPECT_CALL(mockHw, printLog(_, _)).Times(1);

    logger->handleCommand("invalid_cmd_123");
}