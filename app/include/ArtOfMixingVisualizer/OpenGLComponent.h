/*
  ==============================================================================

    OpenGLComponent.h
    Created: 24 Nov 2025 12:23:43pm
    Author:  nate

  ==============================================================================
*/

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_opengl/juce_opengl.h>

//==============================================================================
/*
*/
class OpenGLComponent  : public juce::Component,
                         public juce::OpenGLRenderer
{
public:
    OpenGLComponent();
    ~OpenGLComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

private:
    juce::OpenGLContext openGLContext;

    struct Vertex
    {
        float position[2];
        float colour[4];
    };
    
    std::vector<Vertex> vertexBuffer;
    std::vector<unsigned int> indexBuffer;

    GLuint vbo;
    GLuint ibo;

    juce::String vertexShader;
    juce::String fragmentShader;

    std::unique_ptr<juce::OpenGLShaderProgram> shaderProgram;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLComponent)
};
