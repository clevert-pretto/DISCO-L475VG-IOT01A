#include <gtest/gtest.h>
#include <gmock/gmock.h>

/* Application Headers (Logic Only) */
#include "appHeartbeat.hpp"
#include "appDefines.hpp" // Contains logic constants, zero hardware dependencies

using namespace FreeRTOS_Cpp;
using ::testing::Return;
using ::testing::Exactly;

/**
 * @brief Mock for the RTOS Interface.
 * Allows us to simulate event bits and verify task delays.
 */
class MockRtos : public IRTOS {
public:
    MOCK_METHOD(void, delay, (uint32_t ms), (override));
    MOCK_METHOD(uint32_t, getEventBits, (void* handle), (override));
    MOCK_METHOD(void, setEventBits, (void* handle, uint32_t bits), (override));
};

/**
 * @brief Mock for the Hardware Interface.
 * Allows us to verify LED toggles without a physical board.
 */
class MockHardware : public IHardware {
public:
    MOCK_METHOD(void, toggleLed, (uint16_t ledId), (override));
};

/**
 * @brief Test Fixture for Heartbeat logic.
 */
class HeartbeatTest : public ::testing::Test {
protected:
    MockRtos mockRtos;
    MockHardware mockHw;
    
    // Fake pointers to simulate FreeRTOS handles
    void* fakeSysEvents = reinterpret_cast<void*>(0xDEADBEEF);
    void* fakeWdgEvents = reinterpret_cast<void*>(0xC0FFEE);
};

// --- Test Case 1: Operational State ---
TEST_F(HeartbeatTest, BlinksAtOperationalSpeed) {
    AppHeartbeat heartbeat(&mockRtos, &mockHw, fakeSysEvents, fakeWdgEvents);

    // 1. Return 'Success' bit when logic checks system state
    EXPECT_CALL(mockRtos, getEventBits(fakeSysEvents))
        .WillOnce(Return(EVENT_BIT_INIT_SUCCESS));

    // 2. Verify LED toggle is called with the Status LED ID
    EXPECT_CALL(mockHw, toggleLed(HW_ID_STATUS_LED))
        .Times(Exactly(1));

    // 3. Verify delay is exactly 1000ms
    EXPECT_CALL(mockRtos, delay(1000))
        .Times(Exactly(1));

    // 4. Verify Watchdog bit is set
    EXPECT_CALL(mockRtos, setEventBits(fakeWdgEvents, WATCHDOG_BIT_HEARTBEAT))
        .Times(Exactly(1));

    heartbeat.update();
}

// --- Test Case 2: Initialization Failure ---
TEST_F(HeartbeatTest, BlinksFastOnFailure) {
    AppHeartbeat heartbeat(&mockRtos, &mockHw, fakeSysEvents, fakeWdgEvents);

    // 1. Simulate a failure state
    EXPECT_CALL(mockRtos, getEventBits(fakeSysEvents))
        .WillOnce(Return(EVENT_BIT_INIT_FAILED));

    // 2. Verify delay is 500ms (Faster blink)
    EXPECT_CALL(mockRtos, delay(500))
        .Times(Exactly(1));

    heartbeat.update();
}

// --- Test Case 3: Default/Unknown State ---
TEST_F(HeartbeatTest, BlinksVeryFastOnUnknownState) {
    AppHeartbeat heartbeat(&mockRtos, &mockHw, fakeSysEvents, fakeWdgEvents);

    // 1. Simulate no bits set (Neither success nor fail)
    EXPECT_CALL(mockRtos, getEventBits(fakeSysEvents))
        .WillOnce(Return(0U));

    // 2. Verify delay is 100ms (Fault/Warning blink)
    EXPECT_CALL(mockRtos, delay(100))
        .Times(Exactly(1));

    heartbeat.update();
}