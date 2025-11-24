cmake_minimum_required(VERSION 3.22)

project(ArtOfMixingVisualizer VERSION 0.0.1)

juce_add_gui_app(${PROJECT_NAME}
    VERSION 0.0.1      
    ICON_BIG icons/icon_small.png
    ICON_SMALL icons/icon_small.png
    DOCUMENT_EXTENSIONS ""
    COMPANY_NAME "NathanielDAnnnunzio"
    PRODUCT_NAME "ArtofMixingVisualizer")     

target_sources(${PROJECT_NAME}
    PRIVATE
        source/Main.cpp
        source/MainComponent.cpp
        source/OpenGLComponent.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        JUCE_WEB_BROWSER=0 
        JUCE_USE_CURL=0    
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_VERSION>")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
