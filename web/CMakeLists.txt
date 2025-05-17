#
# Creates the web-folder
#
# Mainly it creates def_signals.js and others
# from the .json files.
#

find_package (Python3 COMPONENTS Interpreter)

# creates serialization.cpp with a script.
# note: we create it in the source dir and check it in, so that not everybody
#   has to have python and cmake running.
if (${Python3_FOUND})

    # -- generate def_signals.js
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_signals.js
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/signals_config.json
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_signals.js
            "defSignals"
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/signals_config.json
    )

    add_custom_target (generate_signals_json DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_signals.js
    )

    # -- generate def_procs.js
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_procs.js
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/procs_config.json
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_procs.js
            "defProcs"
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/procs_config.json
    )

    add_custom_target (generate_procs_json DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_procs.js
    )

    # -- generate def_sample.js
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_sample.js
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/sample_config.json
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_sample.js
            "defSample"
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/sample_config.json
    )

    add_custom_target (generate_audio_json DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_sample.js
    )

    # -- generate def_config.js
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_config.js
        COMMAND
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../configs/full_boat.json
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_config.js
            "defConfig"
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/create_js.py
            ${CMAKE_CURRENT_SOURCE_DIR}/../configs/full_boat.json
    )

    add_custom_target (generate_config_json DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/script/def_config.js
    )
endif ()


add_custom_target (web
    ALL
    DEPENDS
        generate_signals_json
        generate_procs_json
        generate_audio_json
        generate_config_json
)
