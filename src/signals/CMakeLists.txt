#
# CMake file for the rc_signals library for the rc functions controller.
#

find_package (Python3 COMPONENTS Interpreter)

# creates serialization.cpp with a script.
# note: we create it in the source dir and check it in, so that not everybody
#   has to have python and cmake running.
if (${Python3_FOUND})
    add_custom_command (
        OUTPUT
            # ${CMAKE_CURRENT_SOURCE_DIR}/serialization.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/signal_types.h
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/signals_tool.py
            --header ${CMAKE_CURRENT_SOURCE_DIR}/signal_types.h
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/signals_tool.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../config/signals_config.json
    )
    add_custom_target (generate_signal_types_h DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/signal_types.h)
endif ()


if (${ESP_PLATFORM})  # idf build system
    # no subdirectory
    idf_component_register(SRCS
        signals.cpp
        INCLUDE_DIRS ".")

   add_dependencies (${COMPONENT_LIB} generate_signal_types_h)

else ()

    add_library (rc_signals
        signals.cpp
    )
    add_dependencies (rc_signals generate_signal_types_h)

    target_include_directories (rc_signals
        PUBLIC
            .
    )
endif ()
