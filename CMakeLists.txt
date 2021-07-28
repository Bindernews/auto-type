cmake_minimum_required(VERSION 3.16)

OPTION(CFG_KEYBOARD_OUT "Act as a USB keyboard and type text" ON)
OPTION(CFG_UART_OUT "Useful for debugging. Print to the Pico's UART." OFF)

OPTION(CFG_BUTTON_CONTROLS "Support 3-button controls to manipulate what is printed." ON)
OPTION(CFG_DEFAULT_CONTROLS "Print out every piece of data consecutively." OFF)

include(pico_sdk_import.cmake)

project(auto_type LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

## This helper function converts a text file into a C array.
# INPUT : Input file (absolute path)
# OUTPUT : Output file path WITHOUT the file extension
# NAME : Name of the array variable
# TARGETS : List of cmake targets to add dependencies for
function(add_data_source)
    cmake_parse_arguments(args "" "INPUT;OUTPUT;NAME" "TARGETS" ${ARGN})
    add_custom_command(
        OUTPUT ${args_OUTPUT}.c
        COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/dataconvert.py
            ${args_INPUT} ${args_OUTPUT} ${args_NAME}
        DEPENDS ${args_INPUT} ${CMAKE_CURRENT_LIST_DIR}/dataconvert.py
        VERBATIM)
    foreach(target IN LISTS args_TARGETS)
        target_sources(${target} PRIVATE ${args_OUTPUT}.c)
    endforeach()
endfunction()

add_executable(auto_type)

#######################
# SECTION Add Sources #
#######################

add_data_source(
    INPUT ${CMAKE_CURRENT_LIST_DIR}/sources/example1.txt
    OUTPUT ${CMAKE_BINARY_DIR}/example1_bin
    NAME EXAMPLE1_ARRAY
    TARGETS auto_type)

add_data_source(
    INPUT ${CMAKE_CURRENT_LIST_DIR}/sources/example2.txt
    OUTPUT ${CMAKE_BINARY_DIR}/example2_bin
    NAME EXAMPLE2_ARRAY
    TARGETS auto_type)

###########################
# END SECTION Add Sources #
###########################

target_sources(auto_type PRIVATE
    config_check.h
    main.cpp
    map_key.c
    decode.c
    decode.h
    lib/miniz/miniz.c)

target_link_libraries(auto_type PUBLIC
    pico_stdlib
    pico_time
    pico_unique_id
    hardware_gpio)

target_include_directories(auto_type PUBLIC
    .
    lib/miniz
    ${CMAKE_BINARY_DIR})

target_compile_definitions(auto_type PRIVATE
    "PICO_STDIO_DEFAULT_CRLF=1"
    "CFG_BUTTON_CONTROLS")

if (CFG_KEYBOARD_OUT)
    target_sources(auto_type PRIVATE
        usb_descriptors.c)
    target_link_libraries(auto_type PUBLIC
        tinyusb_device
        tinyusb_board)
    target_compile_definitions(auto_type PRIVATE
        "KEYBOARD_OUT")
elseif (CFG_UART_OUT)
    target_compile_definitions(auto_type PRIVATE
        "STDIO_OUT")
    pico_enable_stdio_uart(auto_type 1)
endif()

if (CFG_BUTTON_CONTROLS)
    target_compile_definitions(auto_type PRIVATE
        "CFG_BUTTON_CONTROLS")
elseif (CFG_DEFAULT_CONTROLS)
    target_compile_definitions(auto_type PRIVATE
        "CFG_DEFAULT_CONTROLS")
endif()

# Never use USB for output
pico_enable_stdio_usb(auto_type 0)

# create map/bin/hex files
pico_add_extra_outputs(auto_type)


