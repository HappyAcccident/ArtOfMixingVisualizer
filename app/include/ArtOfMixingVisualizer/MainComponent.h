#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "ArtOfMixingVisualizer/OpenGLComponent.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent final : public juce::AudioAppComponent,
                            public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    juce::String getState();

private:
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };

    void changeState(TransportState newState);
    void openButtonClicked(int button);
    void playButtonClicked();
    void stopButtonClicked();
    void toggleButtonStateChanged(int button);


    std::array<std::unique_ptr<juce::TextButton>, 5> openButtons;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    std::array<std::unique_ptr<juce::ToggleButton>, 5> toggleButtons;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::array<std::unique_ptr<juce::AudioFormatReaderSource>, 5> readerSources;
    std::array<std::unique_ptr<juce::AudioTransportSource>, 5> transportSources;
    juce::MixerAudioSource mixer;
    TransportState state;

    OpenGLComponent openGLComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
