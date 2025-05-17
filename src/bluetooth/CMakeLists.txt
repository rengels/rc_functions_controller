#
# CMake file for the bluetooth library for the rc functions controller.
#

if (${ESP_PLATFORM})  # idf build system
    idf_component_register(SRCS
            main_srv.cpp
            gatt_svr.c

            nimble_peripheral_utils/misc.c
       INCLUDE_DIRS "." "nimble_peripheral_utils" PRIV_REQUIRES bt)

    add_compile_definitions(
            CONFIG_EXAMPLE_IO_TYPE=3
    )

else ()

    if (${ARDUINO})
        # Can only be compiled for arduino since it contains
        # HW specific stuff

        add_library(rc_bluetooth
            main_srv.cpp
            gatt_svr.c

            nimble_peripheral_utils/misc.c
        )

        target_compile_definitions(rc_bluetooth
            PRIVATE
                CONFIG_EXAMPLE_IO_TYPE=3
        )

        target_include_directories(rc_bluetooth
            PUBLIC
                .
            PRIVATE
                nimble_peripheral_utils
        )

        target_link_libraries(rc_bluetooth
            PRIVATE
                idf::freertos
                idf::bt

                rc_signals
        )
    endif()
endif()
