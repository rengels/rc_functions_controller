#
# CMake file for the output library for the rc functions controller.
#

if (${ESP_PLATFORM})  # idf build system
    idf_component_register(SRCS
            output.cpp
            output_audio.cpp
            output_can.cpp
            output_dummy.cpp
            output_esc.cpp
            output_led.cpp
            output_pwm.cpp
        INCLUDE_DIRS "."
        REQUIRES
            esp_timer
            driver
            esp_driver_twai

            audio
            signals
            proc
        )
    target_compile_definitions (${COMPONENT_LIB}
        PRIVATE
            HAVE_NV
            ARDUINO
    )

else ()

    add_library (rc_output
        output_dummy.cpp
    )
    target_include_directories (rc_output
        PUBLIC
            .
    )
    target_link_libraries (rc_output
        PUBLIC
            rc_signals
            rc_proc
            rc_audio
    )

    if (${ARDUINO})
        # The following only be compiled for esp32 since it contains
        # HW specific stuff
        target_compile_definitions (rc_controller
            PRIVATE
                HAVE_NV
                ARDUINO
        )
        target_sources (rc_output
            PRIVATE
                output_can.cpp
                output_esc.cpp
                output_led.cpp
                output_pwm.cpp
                output_audio.cpp
        )
        target_link_libraries (rc_output
            PRIVATE
                idf::freertos
                idf::driver
                idf::bt
        )
    endif()
endif()

