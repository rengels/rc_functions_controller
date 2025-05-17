#
# CMake file for the output library for the rc functions controller.
#

set(audio_srcs
    audio_dynamic.cpp
    audio_engine.cpp
    audio_loop.cpp
    audio_noise.cpp
    audio_ringbuffer.cpp
    audio_simple.cpp
    audio_steam.cpp
)

if (${ESP_PLATFORM})  # idf build system

    idf_component_register (
        SRCS ${audio_srcs}
        INCLUDE_DIRS
        .
        PRIV_REQUIRES
        signals
        proc
    )

else ()

    add_library (rc_audio
        ${audio_srcs}
    )
    target_include_directories (rc_audio
        PUBLIC
            .
    )
    target_link_libraries (rc_audio
        PUBLIC
            rc_signals
            rc_proc
    )
endif ()
