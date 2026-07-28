#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectralEqAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer,
                                        private juce::TextEditor::Listener
{
public:
    SpectralEqAudioProcessorEditor (SpectralEqAudioProcessor&);
    ~SpectralEqAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void textEditorReturnKeyPressed (juce::TextEditor& editor) override;
    void processConsoleCommand (const juce::String& command);

    SpectralEqAudioProcessor& audioProcessor;

    std::array<float, SpectralEqAudioProcessor::fftSize / 2 + 1> guiScopeData;
    
    juce::TextEditor commandInput;
    int bootStep = 0;
    bool showHelpOverlay = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEqAudioProcessorEditor)
};
