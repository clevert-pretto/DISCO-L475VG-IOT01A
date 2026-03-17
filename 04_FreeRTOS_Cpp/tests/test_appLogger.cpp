#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "appLogger.hpp"
#include "appDefines.hpp"
/* Include the Shared Mocks */
#include "MockInterfaces.hpp"

using namespace FreeRTOS_Cpp;
using ::testing::Return;
using ::testing::_;
using ::testing::HasSubstr;

class AppLoggerTest : public ::testing::Test {
protected:
    MockRtos mockRtos;
    MockHardware mockHw;
    appLogger* logger;

    void SetUp() override {
        logger = new appLogger(&mockRtos, &mockHw, nullptr, nullptr, nullptr);
        // Initialize with dummy queues/mutexes so it passes null checks
        logger->init((void*)1, (void*)2, (void*)3, (void*)4, (void*)5, (void*)6);
    }

    void TearDown() override {
        delete logger;
    }
};

// Test Case 1: Command Handling Routing
TEST_F(AppLoggerTest, HandlesDumpCommandCorrectly) {
    // 1. PRIME THE DATA: Set address so the loop runs at least once
    // We use Solution 3 (Macro) to access this private member
    logger->_u32CurrentWriteAddress = LOG_DATA_START + LOG_ENTRY_SIZE;

    // 2. EXPECT MUTEXES: Dump logs takes QSPI and UART mutexes
    EXPECT_CALL(mockRtos, takeMutex(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mockRtos, giveMutex(_)).WillRepeatedly(Return(true));

    // 3. EXPECT LOGGING: logMessage() at the end of dumpLogs calls queueSend
    // We expect it to be called for the "--- FLASH LOG DUMP END ---" message
    EXPECT_CALL(mockRtos, queueSend((void*)1, testing::_, 0))
        .WillOnce(Return(true));

    // 4. EXPECT UART PRINT: The header and at least one data point
    EXPECT_CALL(mockHw, printLog(testing::NotNull(), testing::_))
        .Times(testing::AtLeast(2)); 
    
    // 5. SIMULATE STORAGE: Ensure storageRead returns true so the loop processes
    EXPECT_CALL(mockHw, storageRead(testing::_, testing::_, testing::_))
        .WillRepeatedly(Return(true));

    // Execute the 'd' (dump) command
    logger->handleCommand('d');
}
// Test Case 2: Invalid Command Handling
TEST_F(AppLoggerTest, RejectsInvalidCommands) {
    // Should take UART mutex to print "Invalid command"
    EXPECT_CALL(mockRtos, takeMutex(_, _)).WillOnce(Return(true));
    EXPECT_CALL(mockRtos, giveMutex(_)).WillOnce(Return(true));
    
    // QSPI Mutex should NOT be taken for an invalid command
    EXPECT_CALL(mockRtos, takeMutex((void*)5, _)).Times(0);

    // We can't easily check the string contents directly without a custom matcher,
    // but we verify the hardware receives a print call.
    EXPECT_CALL(mockHw, printLog(testing::NotNull(), _)).Times(1);

    logger->handleCommand('x'); // 'x' is invalid
}