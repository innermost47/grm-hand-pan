# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Anthony Charretier
cmake_minimum_required(VERSION 3.22)
project(GRMHandPan VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GRMH_BUILD_PLUGIN "Build the JUCE plugin, VST3 and standalone (downloads JUCE)" ON)
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
    grmh_add_core_test(test_layout field_layout)

    # the offline tools: the core's sound to a WAV file for the ear, and the probes of the networks
    foreach(tool render_scale render_melody probe_click probe_sympathy measure_recordings)
        add_executable(${tool} tools/${tool}.cpp)
        target_link_libraries(${tool} PRIVATE cppgrm)
    endforeach()
endif()

if(GRMH_BUILD_PLUGIN)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(GRMHandPan
        COMPANY_NAME "InnerMost47"
        BUNDLE_ID "com.innermost47.grm-hand-pan"
        PLUGIN_MANUFACTURER_CODE Grmc
        PLUGIN_CODE Grmh
        FORMATS VST3 Standalone
        PRODUCT_NAME "GRM Hand Pan"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(GRMHandPan PRIVATE
        plugin/HandPanProcessor.cpp
        plugin/SolverThread.cpp)

    target_compile_definitions(GRMHandPan PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(GRMHandPan
        PRIVATE
            cppgrm
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    if(GRMH_BUILD_TESTS)
        # a test built against the plugin's code
        function(grmh_add_plugin_test source ctest_name)
            add_executable(${source} tests/${source}.cpp)
            target_link_libraries(${source} PRIVATE GRMHandPan cppgrm juce::juce_audio_utils
                juce::juce_recommended_config_flags)
            target_compile_definitions(${source} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
            add_test(NAME ${ctest_name} COMMAND ${source})
        endfunction()

        grmh_add_plugin_test(plugin_host_test plugin_as_hosted)
    endif()
endif()
