
# Create documentation
#
# for the documentation to be created you need cmake to be able
# to find doxygen

find_package (Doxygen)
if (DOXYGEN_FOUND)

    find_package (Python3 COMPONENTS Interpreter)

    # creates some files with a script.
    # note: we create it in the source dir and check it in, so that not everybody
    #   has to have python and cmake running.
    if (Python3_FOUND)
        add_custom_command (
            OUTPUT
                ${CMAKE_CURRENT_SOURCE_DIR}/procs.md
                ${CMAKE_CURRENT_SOURCE_DIR}/signals.md
            COMMAND
                ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/create_proc_table.py
                --procs-table ${CMAKE_CURRENT_SOURCE_DIR}/procs.md
                --signals-table ${CMAKE_CURRENT_SOURCE_DIR}/signals.md
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/create_proc_table.py
                ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/procs_config.json
                ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/signals_config.json
        )
        add_custom_target (generated_md_files DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/procs.md
            ${CMAKE_CURRENT_SOURCE_DIR}/signals.md
        )
    endif()

    find_file (DOXYGEN_PLANTUML_JAR_PATH
        plantuml.jar
        PATHS
        "/usr/share/plantuml"
    )

    set (DOXYGEN_INCLUDE_PATH "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include")

    set (DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set (DOXYGEN_CALL_GRAPH NO)
    set (DOXYGEN_CALLER_GRAPH NO)
    set (DOXYGEN_COLLABORATION_GRAPH NO)

    set (DOXYGEN_ENABLE_PREPROCESSING YES)
    set (DOXYGEN_MACRO_EXPANSION YES)
    set (DOXYGEN_EXPAND_ONLY_PREDEF YES)
    set (DOXYGEN_EXPAND_AS_DEFINED "TEST" "GTEST_TEST" "GTEST_TEST_" "GTEST_TEST_CLASS_NAME_")

    doxygen_add_docs (doxygen
        .
        ../src
        ../test
        ../README.md
        COMMENT "Generate Documentation"
    )
    add_dependencies (doxygen
        generated_md_files
    )
endif()
