#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent final : public juce::AudioAppComponent,
                            public juce::ChangeListener,
                            public juce::Timer
{
public:
    //==============================================================================
    MainComponent();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

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

    enum Instrument
    {
        Violin_I,
        Violin_II,
        Viola,
        Chello_I,
        Chello_II
    };

    std::vector<Instrument> instruments = {Violin_I,
                                           Violin_II,
                                           Viola,
                                           Chello_I,
                                           Chello_II};
    
    static juce::String instrumentToString(Instrument instrument)
    {
        switch (instrument)
        {
            case Instrument::Violin_I:   return "Violin I";
            case Instrument::Violin_II:   return "Violin II";
            case Instrument::Viola:   return "Violia";
            case Instrument::Chello_I:   return "Chello I";
            case Instrument::Chello_II:   return "Chello II";
        }
        jassertfalse; // catches invalid values in debug builds
        return "Unknown";
    }

    void changeState(TransportState newState);
    void openButtonClicked(Instrument instrument);
    void playButtonClicked();
    void stopButtonClicked();


    std::array<std::unique_ptr<juce::TextButton>, 5> openButtons;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::array<std::unique_ptr<juce::AudioFormatReaderSource>, 5> readerSources;
    std::array<std::unique_ptr<juce::AudioTransportSource>, 5> transportSources;
    juce::MixerAudioSource mixer;
    TransportState state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
