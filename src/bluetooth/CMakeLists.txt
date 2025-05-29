#
# CMake file for the bluetooth library for the rc functions controller.
#

if (${ESP_PLATFORM})  # idf build system
    idf_component_register(SRCS
            main_srv.cpp
            gap.c
            gatt_svc.c

       INCLUDE_DIRS "."
       PRIV_REQUIRES
            bt
            nvs_flash
       )

    add_compile_definitions(
            CONFIG_EXAMPLE_IO_TYPE=3
    )

else ()

    if (${ARDUINO})
        # Can only be compiled for arduino since it contains
        # HW specific stuff

        add_library(rc_bluetooth
            main_srv.cpp
            gap.c
            gatt_svc.c
        )

        target_include_directories(rc_bluetooth
            PUBLIC
                .
        )

        target_link_libraries(rc_bluetooth
            PRIVATE
                idf::freertos
                # idf::bt
                idf::nvs_flash

                rc_signals
        )
    endif()
endif()
