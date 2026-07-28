#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <complex>

struct DynamicBand
{
    std::atomic<float> frequency { 100.0f };
    std::atomic<float> dynTarget { 6.0f };
    std::atomic<float> gainOffset { 0.0f };
    std::atomic<float> threshold { -12.0f };
    std::atomic<bool> active { false };
};

class SpectralEqAudioProcessor : public juce::AudioProcessor
{
public:
    SpectralEqAudioProcessor();
    ~SpectralEqAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HitechNoLifeEq"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int fftOrder = 13; // 8192 bin FFT
    static constexpr int fftSize = 1 << fftOrder;

    std::array<std::atomic<float>, fftSize / 2 + 1> binGains;
    std::array<float, fftSize / 2 + 1> magnitudeBuffer;
    juce::SpinLock spectrumLock;

    std::atomic<float> currentRms { 0.0f };
    std::atomic<float> currentPeak { 0.0f };
    std::atomic<float> dominantFreq { 0.0f };

    std::array<DynamicBand, 3> dynamicBands;
    std::atomic<bool> modsEnabled { false };

private:
    void processSpectralFrame();

    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize * 2> fftData;
    std::array<float, fftSize> fifo;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEqAudioProcessor)
};
