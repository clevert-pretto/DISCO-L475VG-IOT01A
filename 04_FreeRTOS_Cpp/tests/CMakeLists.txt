include(FetchContent)

# Automatically download GoogleTest from GitHub
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

# For Windows/WSL compatibility, prevent GTest from overriding your runtime settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Define the test runner executable
add_executable(unit_tests 
    test_xtoa.cpp
    test_appHeartbeat.cpp 
    ../App/Src/xtoa.cpp # Link the actual logic being tested
    ../App/Src/appHeartbeat.cpp # Link the actual logic being tested
)

target_compile_definitions(unit_tests PRIVATE UNIT_TEST)

# Link against GTest
target_link_libraries(unit_tests PRIVATE 
  GTest::gtest_main
  GTest::gmock
)

# Include paths for the headers being tested
target_include_directories(unit_tests PRIVATE
    ../App/Inc
    ../Interfaces/Inc
 )

# Enable coverage flags for the host build only
target_compile_options(unit_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
target_link_options(unit_tests PRIVATE --coverage)


# Add to the CTest registry
add_test(NAME XtoaUtilityTests COMMAND unit_tests)

# Create the reports directory during the build process
add_custom_command(TARGET unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Reports
    COMMENT "Creating reports directory for test results..."
)