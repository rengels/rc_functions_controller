#
# CMake file for the rc engine controller.
#


# creates serialization.cpp with a script.
# note: we create it in the source dir and check it in, so that not everybody
#   has to have python and cmake running.
find_package (Python3 COMPONENTS Interpreter)
if (${Python3_FOUND})
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/serialization.cpp
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/serialization_tool.py
            --cpp ${CMAKE_CURRENT_SOURCE_DIR}/serialization.cpp
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/serialization_tool.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../config/procs_config.json
            ${CMAKE_CURRENT_SOURCE_DIR}/../config/signals_config.json
    )
    add_custom_target (generate_serialization_cpp DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/serialization.cpp)
endif()


if (${ESP_PLATFORM})
    idf_component_register(
        SRCS
            main.cpp
            flash_sample.cpp
            proc_storage.cpp
            sample_storage_singleton.cpp
            serialization.cpp
            simple_byte_stream.cpp
            wav_sample.cpp
        INCLUDE_DIRS "."
        PRIV_REQUIRES
            nvs_flash
            spi_flash
            esp_timer

            signals
            proc
            engine
            input
            output
            audio
            samples

            driver
            bluetooth)
    target_compile_definitions (${COMPONENT_LIB}
        PRIVATE
            HAVE_NV
            ARDUINO
    )

    if (${Python3_FOUND})
       add_dependencies (${COMPONENT_LIB} generate_serialization_cpp)
    endif()

else ()

    # non esp32 specific stuff goes here
    add_library (rc_controller
        flash_sample.cpp
        proc_storage.cpp
        sample_storage_singleton.cpp
        serialization.cpp
        simple_byte_stream.cpp
        wav_sample.cpp
    )
    target_link_libraries (rc_controller
        PUBLIC
            rc_signals
            rc_proc
            rc_engine
            rc_input
            rc_output
            rc_samples
            rc_audio
    )
    if (${ARDUINO})
        target_compile_definitions (rc_controller
            PRIVATE
                HAVE_NV
                ARDUINO
        )
        target_link_libraries (rc_controller
            PRIVATE
                idf::nvs_flash
                idf::driver
        )
    endif ()


    # main executable
    if (${ARDUINO})
        idf_build_process (esp32
            COMPONENTS freertos esptool_py bt nvs_flash esp_timer
        )

        # main executable
        add_executable (rc_functions_controller.elf
            main.cpp
        )

        target_link_libraries (rc_functions_controller.elf
            PRIVATE
                idf::freertos
                rc_signals
                rc_proc
                rc_engine
                rc_input
                rc_output
                rc_bluetooth
                rc_samples
        )

        # Let the build system know what the project executable is to attach more targets, dependencies, etc.
        idf_build_executable (rc_functions_controller.elf)
    endif ()
endif ()
