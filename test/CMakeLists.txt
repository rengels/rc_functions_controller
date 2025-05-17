#
# CMake file for unit test cases.
#
# We don't use the rc_tools library for tests, instead
# we compile the "sources-under-test" files again.
#

# gtest only works for Posix and Windows systems.
# So don't try to compile it for Arduino
if (NOT "${CMAKE_SYSTEM_NAME} " STREQUAL "Generic ")

    enable_testing()

    # GoogleTest requires at least C++14 but we use span from C++ 20
    set (CMAKE_CXX_STANDARD 20)
    set (CMAKE_CXX_STANDARD_REQUIRED ON)

    include (FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable (googletest)

    include(GoogleTest)

    # -- Controller test
    # testing the functionality and classes in controller/main
    add_custom_command (
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/dummy_wav.obj
        COMMAND
            ld -r -b binary dummy.wav -z noexecstack -o "${CMAKE_CURRENT_BINARY_DIR}/dummy_wav.obj"
        COMMAND
            objcopy --rename-section .data=.rodata,alloc,load,readonly,data,contents "${CMAKE_CURRENT_BINARY_DIR}/dummy_wav.obj"
        DEPENDS
            dummy.wav
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable (controller_test
        bytestream_test.cpp
        proc_storage_test.cpp
        sample_storage_test.cpp
        wav_sample_test.cpp
        flash_sample_test.cpp
        dummy_wav.obj
    )
    target_include_directories (controller_test
        PRIVATE
            .
            ${CMAKE_SOURCE_DIR}/src/controller
    )
    target_link_libraries (controller_test
        PRIVATE
            GTest::gtest_main
            rc_audio
            rc_samples
            rc_controller
            rc_input
            rc_proc
    )
    add_test (controller_test controller_test)


    # -- proc test
    add_executable (proc_test
        engine_gear_test.cpp
        engine_idle_test.cpp
        engine_reverse_test.cpp
        engine_simple_test.cpp
        engine_speed_test.cpp
        proc_auto_test.cpp
        proc_sequence_test.cpp
        proc_switch_test.cpp
        proc_test.cpp
    )
    target_link_libraries (proc_test
        PUBLIC
            GTest::gtest_main
            rc_proc
            rc_engine
            rc_input
    )
    target_include_directories (proc_test
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src/controller
    )
    add_test (proc_test proc_test)


    # -- curves test
    add_executable (curves_test
      curves_test.cpp
    )
    target_link_libraries (curves_test
        PUBLIC
            GTest::gtest_main
            rc_proc
    )
    add_test (curves_test curves_test)


    # -- audio test
    add_executable (audio_test
      audioringbuffer_test.cpp
      audio_test.cpp
    )
    target_link_libraries (audio_test
        PUBLIC
            GTest::gtest_main
            rc_audio
            rc_signals
    )
    add_test (audio_test audio_test)


    # -- engine simulation tool
    # boost program_options for the engine emulator
    find_package(Boost 1.30 COMPONENTS program_options)
    if(Boost_FOUND)
        # target_include_directories(test PRIVATE ${Boost_INCLUDE_DIRS})
        add_executable (engine_simulation
            engine_simulation.cpp
        )
        target_link_libraries (engine_simulation
            PUBLIC
                rc_proc
                rc_input
                rc_engine
                ${Boost_LIBRARIES}
        )
    else ()
        message ("Boost not found, engine_simulator will not be compiled.")
    endif()

endif ()
