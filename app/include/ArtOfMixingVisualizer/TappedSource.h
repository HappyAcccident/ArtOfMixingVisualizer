#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_dsp/juce_dsp.h>
#include "ArtOfMixingVisualizer/OpenGLComponent.h"

class TappedSource : public juce::AudioSource
{
public:
    TappedSource(juce::AudioTransportSource& s, OpenGLComponent& gl) : audioTransportSource(s), 
                                                                       openGLComponent(gl) {}

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        audioTransportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override
    {
        audioTransportSource.releaseResources();
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override
    {
        audioTransportSource.getNextAudioBlock(info);

        if (info.buffer->getNumChannels() > 0)
        {
            auto* data = info.buffer->getReadPointer(0, info.startSample);
            for (int i = 0; i < info.numSamples; ++i)
                openGLComponent.pushNextSampleIntoFifo(data[i]);
        }
    }

private:
    juce::AudioTransportSource& audioTransportSource;
    OpenGLComponent& openGLComponent;
};
