#include "ArtOfMixingVisualizer/MainComponent.h"

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    // g.drawText (this->getTransportSourceState() + this->getState(), getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    openButton.setBounds(getWidth()/50, 1*getHeight()/25, getWidth()/6, 20);
    playButton.setBounds(getWidth()/50, 2*getHeight()/25, getWidth()/6, 20);
    stopButton.setBounds(getWidth()/50, 3*getHeight()/25, getWidth()/6, 20);
}

void MainComponent::timerCallback()
{
    repaint();
}
