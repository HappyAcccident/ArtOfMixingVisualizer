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
        for(auto instrument : instruments)
        {
            openButtons[instrument] = std::make_unique<juce::TextButton>();
            addAndMakeVisible(openButtons[instrument].get());
            openButtons[instrument]->setButtonText("Choose " + instrumentToString(instrument) + "...");
            openButtons[instrument]->onClick = [this, instrument] {openButtonClicked(instrument);};
        }

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
        for(auto& source : transportSources)
        {
            source = std::make_unique<juce::AudioTransportSource>();
            source->addChangeListener(this);
            mixer.addInputSource(source.get(), true);
        }
        setAudioChannels(0, 10);
    }

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (!std::any_of(readerSources.begin(), readerSources.end(),
                 [](const auto& rs){ return rs != nullptr; }))
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        mixer.getNextAudioBlock(bufferToFill);
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override
    {
        mixer.releaseResources();
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        for (auto& transportSource : transportSources)
        {
                if (transportSource->isPlaying())
                {
                    changeState(Playing);
                    return;
                }
        }

        if ((state == Stopping) || (state == Playing))
        {
            changeState(Stopped);
        }
        else if (state == Pausing)
        {
            changeState(Paused);
        }
    }

    // juce::String getTransportSourceState(Instrument instrument)
    // {
    //     if (transportSources[Instrument].isPlaying())
    //         return "True";
    //     else
    //         return "False";
    // }

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
                    for (auto& transportSource : transportSources)
                    {
                        transportSource->setPosition(0.0);
                    }
                    break;
                case Starting:
                    for (auto& transportSource : transportSources)
                    {
                        transportSource->start();
                    }
                case Playing:
                    playButton.setButtonText ("Pause");
                    stopButton.setEnabled(true);
                    break;
                case Pausing:
                    for (auto& transportSource : transportSources)
                    {
                        transportSource->stop();
                    }
                    break;
                case Paused:
                    playButton.setButtonText("Resume");
                    break;
                case Stopping:
                    for (auto& transportSource : transportSources)
                    {
                        transportSource->stop();
                    }
                    break;
            }
        }
    }

    void openButtonClicked(Instrument instrument)
    {
        chooser = std::make_unique<juce::FileChooser> ("Select a file to play...",
            juce::File {},
            "*.wav;*.mp3;*.flac");

        auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;
        
        chooser->launchAsync (chooserFlags, [this, instrument] (const juce::FileChooser& fc)
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
                        transportSources[instrument]->setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                        playButton.setEnabled(true);
                        readerSources[instrument].reset(newSource.release());
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
