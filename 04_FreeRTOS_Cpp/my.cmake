# Toolchain setup for ARM Cortex-M4
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

# MCU-specific flags
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -g3 -O0")

set(CMAKE_C_FLAGS "${MCU_FLAGS} -fdata-sections -ffunction-sections" CACHE INTERNAL "C Compiler flags")
set(CMAKE_CXX_FLAGS "${MCU_FLAGS} -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics" CACHE INTERNAL "C++ Compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -specs=nosys.specs -Wl,--gc-sections" CACHE INTERNAL "Linker flags")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)