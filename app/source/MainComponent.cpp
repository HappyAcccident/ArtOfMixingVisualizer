#include "ArtOfMixingVisualizer/MainComponent.h"

//==============================================================================

MainComponent::MainComponent() : state (Stopped)
{
    formatManager.registerBasicFormats();
    setAudioChannels(0, 10);
    for(int n = 0; n < 5; n++)
    {
        transportSources[n] = std::make_unique<juce::AudioTransportSource>();
        transportSources[n]->addChangeListener(this);
        mixer.addInputSource(transportSources[n].get(), true);

        openButtons[n] = std::make_unique<juce::TextButton>();
        addAndMakeVisible(openButtons[n].get());
        openButtons[n]->setButtonText("Choose...");
        openButtons[n]->onClick = [this, n] {openButtonClicked(n);};

        toggleButtons[n] = std::make_unique<juce::ToggleButton>();
        addAndMakeVisible(toggleButtons[n].get());
        toggleButtons[n]->onStateChange = [this, n] {toggleButtonStateChanged(n);};
        toggleButtons[n]->setToggleState(true, juce::dontSendNotification);
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
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText (/*this->getTransportSourceState() +*/ this->getState(), getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    for (int n = 0; n < 5; n++)
    {
        openButtons[n]->setBounds(getWidth()/50, (n+1)*getHeight()/25, getWidth()/6, 20);
        toggleButtons[n]->setBounds(getWidth()/50-30, (n+1)*getHeight()/25, 20, 20);
    }
    playButton.setBounds(getWidth()/50, 6*getHeight()/25, getWidth()/6, 20);
    stopButton.setBounds(getWidth()/50, 7*getHeight()/25, getWidth()/6, 20);
}

void MainComponent::timerCallback()
{
    repaint();
}

//==============================================================================
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (!std::any_of(readerSources.begin(), readerSources.end(),
             [](const auto& rs){ return rs != nullptr; }))
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    mixer.getNextAudioBlock(bufferToFill);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::releaseResources()
{
    mixer.releaseResources();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
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

void MainComponent::changeState(TransportState newState)
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

void MainComponent::openButtonClicked(int button)
{
    chooser = std::make_unique<juce::FileChooser> ("Select a file to play...",
        juce::File {},
        "*.wav;*.mp3;*.flac");
    auto chooserFlags = juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles;
    
    chooser->launchAsync (chooserFlags, [this, button] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File {})
            {
                auto* reader = formatManager.createReaderFor(file);
                if (reader != nullptr)
                {
                    openButtons[button]->setButtonText(file.getFileName());

                    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
                    if (state == Playing)
                    {
                        changeState(Stopping);
                    }
                    else if (state == Paused)
                    {
                        changeState(Stopped);
                    }
                    transportSources[button]->setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                    playButton.setEnabled(true);
                    readerSources[button].reset(newSource.release());
                }
            }
        });
}

void MainComponent::playButtonClicked()
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

void MainComponent::stopButtonClicked()
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

void MainComponent::toggleButtonStateChanged(int button)
{
    auto state = toggleButtons[button]->getToggleState();
    if (state)
    {
        transportSources[button]->setGain(1.0);
    }
    else
    {
        transportSources[button]->setGain(0.0);
    }
}

//==============================================================================

juce::String MainComponent::getState()
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