#
# CMake file for the rc library for the rc engine controller.
#

if (${ESP_PLATFORM})  # idf build system
    idf_component_register(SRCS
        input_adc.cpp
        input_demo.cpp
        input_pin.cpp
        input_ppm.cpp
        input_pwm.cpp
        input_sbus.cpp
        input_srxl.cpp
    INCLUDE_DIRS "."
    REQUIRES
        esp_adc

    PRIV_REQUIRES
        bt
        driver
        signals
        proc)

else ()

    add_library (rc_input
        input_demo.cpp
    )
    target_include_directories (rc_input
        PUBLIC
            .
    )
    target_link_libraries(rc_input
        PRIVATE
            rc_signals
            rc_proc
    )

    if (${ARDUINO})
        # Can only be compiled for arduino since it contains
        # HW specific stuff
        target_sources (rc_input
            PRIVATE
                input_adc.cpp
                input_pin.cpp
                input_ppm.cpp
                input_pwm.cpp
                input_sbus.cpp
                input_srxl.cpp
        )

        target_link_libraries(rc_input
            PRIVATE
                idf::freertos
                idf::driver
                esp_adc
        )
    endif()
endif()


