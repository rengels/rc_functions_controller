#
# CMake file for the rc_proc library for the rc functions controller.
#

set(proc_srcs
    proc_auto.cpp
    proc_combine.cpp
    proc_cranking.cpp
    proc_delay.cpp
    proc_direction.cpp
    proc_excavator.cpp
    proc_expo.cpp
    proc_fade.cpp
    proc_indicator.cpp
    proc_map.cpp
    proc_misfire.cpp
    proc_periodic.cpp
    proc_power.cpp
    proc_random.cpp
    proc_sequence.cpp
    proc_scenario.cpp
    proc_switch.cpp
    proc_threshold.cpp
    proc_xenon.cpp
)

if (${ESP_PLATFORM})  # idf build system
    # no subdirectory
    idf_component_register(
        SRCS ${proc_srcs}
        INCLUDE_DIRS "."
        REQUIRES signals)
    add_dependencies (${COMPONENT_LIB} generate_power_curves_h)
    target_compile_definitions (${COMPONENT_LIB}
        PRIVATE
            ARDUINO
    )

else () # build with "normal" cmake

    add_library (rc_proc
        ${proc_srcs}
    )
    target_include_directories (rc_proc
        PUBLIC
            .
    )
    target_link_libraries (rc_proc
        PUBLIC
            rc_signals
    )

endif ()
