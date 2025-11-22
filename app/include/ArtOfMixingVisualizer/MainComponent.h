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
    MainComponent()
        : state (Stopped)
    {
        addAndMakeVisible(&openButton);
        openButton.setButtonText ("Open...");
        openButton.onClick = [this] {openButtonClicked();};

        addAndMakeVisible(&playButton);
        playButton.setButtonText ("Play");
        playButton.onClick = [this] {playButtonClicked();};
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
        playButton.setEnabled(false);

        addAndMakeVisible(&stopButton);
        stopButton.setButtonText ("Stop");
        stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        stopButton.setEnabled(false);
        stopButton.onClick = [this] {stopButtonClicked();};

        setSize (300, 200);
        startTimerHz(60);

        formatManager.registerBasicFormats();
        transportSource.addChangeListener(this);
        setAudioChannels(0, 2);
    }

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (readerSource.get() == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        transportSource.getNextAudioBlock(bufferToFill);
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == &transportSource)
        {
            if (transportSource.isPlaying())
            {
                changeState(Playing);
            }
            else if ((state == Stopping) || (state == Playing))
            {
                changeState(Stopped);
            }
            else if (state == Pausing)
            {
                changeState(Paused);
            }
        }
    }

    juce::String getTransportSourceState()
    {
        if (transportSource.isPlaying())
            return "True";
        else
            return "False";
    }

    juce::String getState()
    {
        switch (state)
        {
            case TransportState::Stopped:   return "Stopped";
            case TransportState::Starting:  return "Starting";
            case TransportState::Playing:   return "Playing";
            case TransportState::Pausing:   return "Pausing";
            case TransportState::Paused:    return "Paused";
            case TransportState::Stopping:  return "Stopping";
        }

        jassertfalse; // catches invalid values in debug builds
        return "Unknown";
    }

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

    void changeState(TransportState newState)
    {
        if (state != newState)
        {
            state = newState;
            switch (state)
            {
                case Stopped:
                    playButton.setButtonText("Play");
                    stopButton.setEnabled(false);
                    transportSource.setPosition(0.0);
                    break;
                case Starting:
                    transportSource.start();
                case Playing:
                    playButton.setButtonText ("Pause");
                    stopButton.setEnabled(true);
                    break;
                case Pausing:
                    transportSource.stop();
                    break;
                case Paused:
                    playButton.setButtonText("Resume");
                    break;
                case Stopping:
                    transportSource.stop();
                    break;
            }
        }
    }

    void openButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser> ("Select a Wave file to play...",
            juce::File {},
            "*.wav;*.mp3");

        auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;
        
        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file != juce::File {})
                {
                    auto* reader = formatManager.createReaderFor(file);
                    if (reader != nullptr)
                    {
                        auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
                        if (state == Playing)
                        {
                            changeState(Stopping);
                        }
                        else if (state == Paused)
                        {
                            changeState(Stopped);
                        }
                        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                        playButton.setEnabled(true);
                        readerSource.reset(newSource.release());
                    }
                }
            });
    }

    void playButtonClicked()
    {
        if ((state == Stopped) || (state == Paused))
        {
            changeState(Starting);
        }
        else if (state == Playing)
        {
            changeState(Pausing);
        }
    }

    void stopButtonClicked()
    {
        if (state == Paused)
        {
            changeState(Stopped);
        }
        else
        {
            changeState(Stopping);
        }
    }


    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
