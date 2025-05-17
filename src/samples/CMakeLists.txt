#
# CMake file for the sample library for the rc functions controller.
#
# This .a file contains binary data for all the static (compiled in) samples.
#

set (audio_files
    ./car/indicator.wav
    ./car/beetle_horn.wav
    ./car/beetle_start.wav
    ./car/siren1.wav
    ./car/siren2.wav
    ./car/siren3.wav
    ./car/beetle1i.wav
    ./car/beetle1r.wav
    ./car/beetle9i.wav
    ./car/beetle9r.wav
    ./ship/bell.wav
    ./ship/horn.wav
    ./train/braking.wav
    ./train/coupling.wav
    ./train/rattle_squeek.wav
    ./train/steam.wav
    ./train/steam_whistle.wav
    ./train/stroke.wav
    ./train/whistle.wav
    ./truck/airbrake.wav
    ./truck/diesel_start.wav
    ./truck/diesel0i.wav
    ./truck/diesel2i.wav
    ./truck/diesel0r.wav
    ./truck/diesel9r.wav
    ./truck/truck_horn.wav
    ./truck/reversing.wav
    ./other/440.wav
    ./other/low_fuel.wav
    ./other/sendling_idle.wav
    ./other/sendling_idle2.wav
    ./other/sendling_idle3.wav
    ./other/sendling_rev.wav
    ./other/sendling_start.wav
    ./other/shot.wav
    ./other/silence.wav
    ./other/track_ratteling.wav
)


# creates sample.cpp with a script.
# note: we create it in the source dir and check it in, so that not everybody
#   has to have python and cmake running.
find_package (Python3 COMPONENTS Interpreter)
if (${Python3_FOUND})
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/audio_tool.py
            --cpp ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/audio_tool.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../config/sample_config.json
    )
    add_custom_target (generate_sample_cpp DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp)
endif()


if (${ESP_PLATFORM})  # idf build system

    idf_component_register (
        SRCS
            sample.cpp
        INCLUDE_DIRS
            .
        EMBED_FILES
            ${audio_files}
        )

    if (${Python3_FOUND})
       add_dependencies (${COMPONENT_LIB} generate_sample_cpp)
    endif()

else ()

    # function to create an obj file out of a wav file
    function (make_obj src_name target_name)

        cmake_path (GET target_name PARENT_PATH target_directory)
        file (MAKE_DIRECTORY "${target_directory}")

        cmake_path (ABSOLUTE_PATH src_name OUTPUT_VARIABLE source_name)
        cmake_path (GET source_name PARENT_PATH source_directory)
        cmake_path (GET source_name FILENAME source_filename)

        add_custom_command (
            OUTPUT
                "${target_name}"
            COMMAND
                ld -r -b binary "${source_filename}" -z noexecstack -o "${target_name}"
                # COMMAND
                # objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents "${target_name}"
            DEPENDS
                "${src_name}"
            WORKING_DIRECTORY
                ${source_directory}
        )
    endfunction ()

    # convert all audio files
    foreach (input IN LISTS audio_files)
        string (REPLACE "\.wav" "_wav.obj" output ${input})
        set (output "${CMAKE_CURRENT_BINARY_DIR}/${output}")
        make_obj(${input} ${output})
        list (APPEND obj_files ${output})
    endforeach ()

    add_library (rc_samples
        sample.cpp
        ${obj_files}
    )
    target_include_directories (rc_samples
        PUBLIC
            .
    )
endif ()

