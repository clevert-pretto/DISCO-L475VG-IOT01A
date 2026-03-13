#include <gtest/gtest.h>
#include "../App/Inc/xtoa.hpp" // Adjust path to your xtoa header

using namespace FreeRTOS_Cpp;

// Test Case 1: Integer to String conversion
TEST(XtoaTest, HandlesPositiveIntegers) {
    char buffer[16];
    xtoa::app_itoa(1234, buffer, 10);
    EXPECT_STREQ(buffer, "1234");
}

// Test Case 2: Float to String conversion (Precision check)
TEST(XtoaTest, HandlesFloatsWithPrecision) {
    char buffer[16];
    // Testing the function used in appLogger::dumpLogs
    xtoa::app_ftoa(3.14159f, buffer, sizeof(buffer)); 
    EXPECT_STREQ(buffer, "3.14");
}

// Test Case 3: Negative numbers
TEST(XtoaTest, HandlesNegativeIntegers) {
    char buffer[16];
    xtoa::app_itoa(-567, buffer, sizeof(buffer));
    EXPECT_STREQ(buffer, "-567");
}