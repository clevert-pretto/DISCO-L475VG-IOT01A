cmake_minimum_required(VERSION 3.16)

project(freeRTOSApp LANGUAGES C CXX ASM)

# Set Standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17) # Transition to Modern C++
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Definitions & Includes
add_definitions(-DSTM32L475xx -DUSE_HAL_DRIVER -DFREERTOS_CPP_APP_RUNNING)

# Source Collection
file(GLOB_RECURSE SOURCES 
    "App/*.cpp"
    "App/*.c"
    "App/Src/*.cpp"
    "App/Src/*.c"
    "Platforms/Stm32/Src/*.cpp"
    "../03_FreeRTOS/Drivers/BSP/B-L475E-IOT01/*.c"
    "../03_FreeRTOS/Drivers/BSP/Components/hts221/*.c"
    "../03_FreeRTOS/Drivers/CMSIS/Device/ST/STM32L4xx/Source/*.c"
    "../03_FreeRTOS/Drivers/STM32L4xx_HAL_Driver/Src/*.c"
    "../03_FreeRTOS/Middlewares/FreeRTOS/*.c"
    "../03_FreeRTOS/Middlewares/FreeRTOS/portable/GCC/ARM_CM4F/*.c"
    "../03_FreeRTOS/Middlewares/FreeRTOS/portable/MemMang/heap_4.c" # Or whichever heap you use
)

# Add the specific startup file
list(APPEND SOURCES "../03_FreeRTOS/Drivers/CMSIS/Device/ST/STM32L4xx/Source/startup_stm32l475xx.s")

# Executable & Linker Script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32L475VGTx_FLASH.ld)

# =========================================================================
# EMBEDDED FIRMWARE TARGET (ARM CROSS-COMPILATION ONLY)
# =========================================================================
if(CMAKE_CROSSCOMPILING)
    message(STATUS "--- CROSS-COMPILING: Building STM32 Firmware ---")

    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Platforms/Stm32/Src/Stm32Platform.cpp")
    
    add_executable(${PROJECT_NAME}.elf ${SOURCES})

    # Apply base flags to everything 
    target_compile_options(${PROJECT_NAME}.elf PRIVATE 
        -Wall 
        -Wextra
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-unused-variable
    )

    # Apply STRICT flags ONLY to C++ (your application code)
    target_compile_options(${PROJECT_NAME}.elf PRIVATE 
        $<$<COMPILE_LANGUAGE:CXX>:-Werror>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
        $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-pragma-once-outside-header>
    )

    # Mark Vendor includes as SYSTEM to ignore their warnings
    target_include_directories(${PROJECT_NAME}.elf SYSTEM PRIVATE    
        ../03_FreeRTOS/Drivers/BSP/B-L475E-IOT01
        ../03_FreeRTOS/Drivers/BSP/Components/hts221
        ../03_FreeRTOS/Drivers/BSP/Components/mx25r6435f
        ../03_FreeRTOS/Drivers/CMSIS/Core/Include
        ../03_FreeRTOS/Drivers/CMSIS/Device/ST/STM32L4xx/Include
        ../03_FreeRTOS/Drivers/STM32L4xx_HAL_Driver
        ../03_FreeRTOS/Drivers/STM32L4xx_HAL_Driver/Inc
        ../03_FreeRTOS/Middlewares/FreeRTOS
        ../03_FreeRTOS/Middlewares/FreeRTOS/include
        ../03_FreeRTOS/Middlewares/FreeRTOS/portable
    )

    target_include_directories(${PROJECT_NAME}.elf PRIVATE
        App
        App/Inc
        Interfaces/Inc
        Platforms/Stm32/Inc
    )

    target_link_options(${PROJECT_NAME}.elf PRIVATE 
        -T${LINKER_SCRIPT}
        -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map
    )

    target_link_libraries(${PROJECT_NAME}.elf PRIVATE c m nosys)

    # Generate Binary File Post-Build
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMENT "Generating ${PROJECT_NAME}.bin from the linked .elf file..."
    )

    # Capture the memory limits from your linker script
    set(FLASH_SIZE 1048576) # 1MB
    set(RAM_SIZE 98304)     # 96KB

    # Create a post-build custom command for the size report
    # add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    #     COMMAND ${CMAKE_SIZE} --format=berkeley ${PROJECT_NAME}.elf
    #     COMMAND ${CMAKE_COMMAND} -E echo "---------------------------------------------------------------"
    #     COMMAND ${CMAKE_COMMAND} -E echo "MEMORY USAGE REPORT"
    #     COMMAND python3 -c "import os; stats=os.popen('${CMAKE_SIZE} ${PROJECT_NAME}.elf').readlines()[1].split(); flash=int(stats[0])+int(stats[1]); ram=int(stats[1])+int(stats[2]); print('FLASH: {} / ${FLASH_SIZE} bytes ({:.2f}%)'.format(flash, flash/${FLASH_SIZE}*100)); print('RAM: {} / ${RAM_SIZE} bytes ({:.2f}%)'.format(ram, ram/${RAM_SIZE}*100))"
    #     COMMAND ${CMAKE_COMMAND} -E echo "---------------------------------------------------------------"
    # )

# =========================================================================
# HOST UNIT TEST TARGET (PC ONLY)
# =========================================================================
else()
    message(STATUS "------ HOST BUILD DETECTED: Building Unit Tests ------")
    
    # Enable CTest and pull in the tests subdirectory
    enable_testing()
    add_subdirectory(tests)
endif()

# =========================================================================
# COMMON STATIC ANALYSIS & FORMATTING
# =========================================================================
find_program(CPPCHECK_PATH cppcheck)
if(CPPCHECK_PATH)
    add_custom_target(lint
        COMMAND ${CPPCHECK_PATH} --enable=all --std=c++17 --inline-suppr
                -I ${CMAKE_CURRENT_SOURCE_DIR}/App/Inc
                ${CMAKE_CURRENT_SOURCE_DIR}/App/Src
                -I ${CMAKE_CURRENT_SOURCE_DIR}/Interfaces/Inc   # Include Interfaces
                -I ${CMAKE_CURRENT_SOURCE_DIR}/Platforms/Stm32/Inc # Include Platforms
                -I ${CMAKE_CURRENT_SOURCE_DIR}/Platforms/Stm32/src # Source Platforms
        COMMENT "Running Cppcheck Static Analysis..."
    )
endif()

find_program(CLANG_FORMAT_PATH clang-format)
if(CLANG_FORMAT_PATH)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/App/*.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/App/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/App/Inc/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Interfaces/Inc/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Platforms/Stm32/Inc/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Platforms/Stm32/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Platforms/Stm32/src/*.hpp)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_PATH} -i ${ALL_CXX_SOURCE_FILES}
        COMMENT "Formatting code to LLVM/Google standard..."
    )
endif()