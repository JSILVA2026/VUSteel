cmake_minimum_required(VERSION 3.22)
project(VUSteel VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VUSteel
    COMPANY_NAME "Local"
    PLUGIN_MANUFACTURER_CODE Lcl1
    PLUGIN_CODE Vust
    FORMATS VST3
    PRODUCT_NAME "VU Steel"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Copia os PNGs com nomes ASCII (sem acento e espaço) para o build
set(VU_ART_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set(VU_ASSET_DIR "${CMAKE_BINARY_DIR}/assets")
configure_file("${VU_ART_DIR}/BOTÃO 01.png"  "${VU_ASSET_DIR}/botao_01.png"  COPYONLY)
configure_file("${VU_ART_DIR}/BOTÃO 02.png"  "${VU_ASSET_DIR}/botao_02.png"  COPYONLY)
configure_file("${VU_ART_DIR}/BOTÃO 03.png"  "${VU_ASSET_DIR}/botao_03.png"  COPYONLY)
configure_file("${VU_ART_DIR}/MEDIDOR_01.png" "${VU_ASSET_DIR}/medidor_01.png" COPYONLY)
configure_file("${VU_ART_DIR}/MEDIDOR_02.png" "${VU_ASSET_DIR}/medidor_02.png" COPYONLY)
configure_file("${VU_ART_DIR}/PANEL_01.png"  "${VU_ASSET_DIR}/panel_01.png"  COPYONLY)

juce_add_binary_data(VUSteelAssets SOURCES
    Source/INTERFACE_OK.png
    "${VU_ASSET_DIR}/botao_01.png"
    "${VU_ASSET_DIR}/botao_02.png"
    "${VU_ASSET_DIR}/botao_03.png"
    "${VU_ASSET_DIR}/medidor_01.png"
    "${VU_ASSET_DIR}/medidor_02.png"
    "${VU_ASSET_DIR}/panel_01.png")

juce_generate_juce_header(VUSteel)

target_sources(VUSteel PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(VUSteel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VUSteel PRIVATE
    VUSteelAssets
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
