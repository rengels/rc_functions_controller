cmake_minimum_required (VERSION 3.23.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (POLICY CMP0135)
  cmake_policy (SET CMP0135 NEW)
endif ()

set (CMAKE_CXX_STANDARD 20) # we use std::clamp and std::span

if (${ESP_PLATFORM})  # idf build system
    include ($ENV{IDF_PATH}/tools/cmake/project.cmake)
    set (EXTRA_COMPONENT_DIRS
        src/audio
        src/bluetooth
        src/controller
        src/engine
        src/input
        src/output
        src/proc
        src/samples
        src/signals
    )
    project (rc_functions_controller CXX C)

    find_package (Python3 COMPONENTS Interpreter)

    # create a merged firmware for ESP Web Tools
    if (${Python3_FOUND})
        add_custom_command (
            OUTPUT
                ${CMAKE_BINARY_DIR}/merged_firmware.bin
            COMMAND
                 ${Python3_EXECUTABLE} -m esptool --chip esp32 merge_bin
                 -o ${CMAKE_BINARY_DIR}/merged_firmware.bin
                  --flash_mode dio
                  --flash_freq 40m
                  --flash_size 4MB
                  0x1000 ${CMAKE_BINARY_DIR}/bootloader/bootloader.bin
                  0x8000 ${CMAKE_BINARY_DIR}/partition_table/partition-table.bin
                  0x10000 ${CMAKE_BINARY_DIR}/rc_functions_controller.bin

            DEPENDS
                ${CMAKE_BINARY_DIR}/bootloader/bootloader.bin
                ${CMAKE_BINARY_DIR}/partition_table/partition-table.bin
                ${CMAKE_BINARY_DIR}/rc_functions_controller.bin
        )
        add_custom_target (merge_firmware DEPENDS
            ${CMAKE_BINARY_DIR}/merged_firmware.bin)
        # add_dependencies (app merge_firmware)
    endif ()

elseif (${ARDUINO}) # build with CMAKE and idf toolchain
    if ("$ENV{IDF_PATH} " STREQUAL " ")
        message(FATAL "IDF_PATH not set. Do \"source export.sh\"")
    endif ()

    include ($ENV{IDF_PATH}/tools/cmake/idf.cmake)
    set (SDKCONFIG_DEFAULTS
        sdkconfig.defaults
    )
    project (rc_functions_controller CXX C)
    add_subdirectory (src)

else () # build for host (e.g. tests with gcc)

    project (rc_functions_controller CXX C)
    add_subdirectory (src)
    add_subdirectory (test)
endif()

add_subdirectory (doc)  # builds the doxygen documentation
add_subdirectory (web)

