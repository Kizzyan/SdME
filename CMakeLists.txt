# Generated CMake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Project name and version
project(Projeto-Final C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Set the Pico board type (e.g., pico or pico_w)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Add the executable
add_executable(Projeto-Final 
    Projeto-Final.c        # Main project file
    tm1637/tm1637.c        # Source file for tm1637 library
)

# Set program name and version (use the same case as in add_executable)
pico_set_program_name(Projeto-Final "Projeto-Final")
pico_set_program_version(Projeto-Final "0.1")

# Include the directory containing tm1637.h
target_include_directories(Projeto-Final PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/tm1637
)

# Link libraries required by the project
target_link_libraries(Projeto-Final
    pico_stdlib    # Standard Pico SDK library
)

# Enable additional outputs (e.g., UF2, hex, bin)
pico_add_extra_outputs(Projeto-Final)

# Enable USB or UART stdio (modify as needed)
pico_enable_stdio_usb(Projeto-Final 1)  # Enable USB output
pico_enable_stdio_uart(Projeto-Final 0) # Disable UART output