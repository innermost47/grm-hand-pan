# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Anthony Charretier
cmake_minimum_required(VERSION 3.22)
project(GRMHandPan VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GRMH_BUILD_TESTS "Build the tests and the offline tools" OFF)

include(FetchContent)
FetchContent_Declare(cppgrm
    GIT_REPOSITORY https://github.com/innermost47/cppgrm.git
    GIT_TAG        main
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(cppgrm)

if(GRMH_BUILD_TESTS)
    enable_testing()

    # a test of the core (no JUCE): tests/<source>.cpp, registered in ctest as <ctest_name>
    function(grmh_add_core_test source ctest_name)
        add_executable(${source} tests/${source}.cpp)
        target_link_libraries(${source} PRIVATE cppgrm)
        add_test(NAME ${ctest_name} COMMAND ${source})
    endfunction()

    grmh_add_core_test(test_scaffold grm_engine_solves)
    grmh_add_core_test(test_constants sourced_constants)
    grmh_add_core_test(test_tone_field tone_field_network)
    grmh_add_core_test(test_cavity cavity_network)
    grmh_add_core_test(test_strike strike_network)
    grmh_add_core_test(test_engine sound_engine)
    grmh_add_core_test(test_radiation mode_radiation)
    grmh_add_core_test(test_participation mode_participation)
    grmh_add_core_test(test_sympathy sympathetic_fields)
    grmh_add_core_test(test_finger_click finger_click)

    # the offline tools: the core's sound to a WAV file, for the ear
    add_executable(render_scale tools/render_scale.cpp)
    target_link_libraries(render_scale PRIVATE cppgrm)
    add_executable(render_melody tools/render_melody.cpp)
    target_link_libraries(render_melody PRIVATE cppgrm)
    add_executable(probe_click tools/probe_click.cpp)
    target_link_libraries(probe_click PRIVATE cppgrm)
    add_executable(probe_sympathy tools/probe_sympathy.cpp)
    target_link_libraries(probe_sympathy PRIVATE cppgrm)
    add_executable(measure_recordings tools/measure_recordings.cpp)
    target_link_libraries(measure_recordings PRIVATE cppgrm)
endif()
