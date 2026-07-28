#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectralEqAudioProcessor::SpectralEqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #ifndef JucePlugin_IsMidiEffect
                      #ifndef JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       forwardFFT (fftOrder),
       window (fftSize, juce::dsp::WindowingFunction<float>::hann)
#endif
{
    for (auto& g : binGains) g.store (1.0f);
    
    dynamicBands[0].frequency.store(100.0f);
    dynamicBands[0].dynTarget.store(5.0f);
    dynamicBands[0].gainOffset.store(-1.0f);
    dynamicBands[0].active.store(true);

    dynamicBands[1].frequency.store(3150.0f);
    dynamicBands[1].dynTarget.store(8.0f);
    dynamicBands[1].gainOffset.store(0.0f);
    dynamicBands[1].active.store(true);

    dynamicBands[2].frequency.store(8000.0f);
    dynamicBands[2].dynTarget.store(4.0f);
    dynamicBands[2].gainOffset.store(1.0f);
    dynamicBands[2].active.store(true);
}

SpectralEqAudioProcessor::~SpectralEqAudioProcessor() {}

void SpectralEqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fifoIndex = 0;
    nextFFTBlockReady = false;
    std::fill (fifo.begin(), fifo.end(), 0.0f);
    std::fill (fftData.begin(), fftData.end(), 0.0f);
}

void SpectralEqAudioProcessor::releaseResources() {}

bool SpectralEqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void SpectralEqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    currentRms.store (buffer.getRMSLevel (0, 0, buffer.getNumSamples()));
    currentPeak.store (buffer.getMagnitude (0, 0, buffer.getNumSamples()));

    auto* channelData = buffer.getReadPointer (0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        fifo[fifoIndex++] = channelData[i];
        if (fifoIndex == fftSize)
        {
            if (!nextFFTBlockReady)
            {
                std::fill (fftData.begin(), fftData.end(), 0.0f);
                std::copy (fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
    }

    if (nextFFTBlockReady)
    {
        processSpectralFrame();
        nextFFTBlockReady = false;
    }
}

void SpectralEqAudioProcessor::processSpectralFrame()
{
    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    forwardFFT.performRealOnlyForwardTransform (fftData.data());

    const int numBins = fftSize / 2 + 1;
    float binWidth = (float) getSampleRate() / (float) fftSize;

    auto* complexData = reinterpret_cast<std::complex<float>*> (fftData.data());

    float maxMag = 0.0f;
    int maxBin = 0;

    for (int bin = 0; bin < numBins; ++bin)
    {
        float mag = std::abs (complexData[bin]);
        if (mag > maxMag)
        {
            maxMag = mag;
            maxBin = bin;
        }

        float currentBinFreq = bin * binWidth;
        float g = binGains[bin].load (std::memory_order_relaxed);

        for (auto& band : dynamicBands)
        {
            if (!band.active.load()) continue;

            if (std::abs (currentBinFreq - band.frequency.load()) < binWidth * 2.0f)
            {
                float db = juce::Decibels::gainToDecibels (mag, -100.0f);
                float thresh = band.threshold.load();

                if (db > thresh)
                {
                    float excess = db - thresh;
                    float dynRatio = band.dynTarget.load();
                    float reductionDb = excess * (1.0f - (1.0f / (dynRatio > 0 ? dynRatio : 1.0f))) + band.gainOffset.load();
                    g *= juce::Decibels::decibelsToGain (-reductionDb);
                }
            }
        }

        complexData[bin] *= g;

        if (bin < magnitudeBuffer.size())
        {
            const juce::SpinLock::ScopedLockType lock (spectrumLock);
            magnitudeBuffer[bin] = std::abs (complexData[bin]);
        }
    }

    dominantFreq.store (maxBin * binWidth);
}

juce::AudioProcessorEditor* SpectralEqAudioProcessor::createEditor()
{
    return new SpectralEqAudioProcessorEditor (*this);
}

void SpectralEqAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void SpectralEqAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralEqAudioProcessor();
}
