#pragma once

#include "AudioProcessorBlock/ThreeTapDelay.h"
#include "AudioProcessorBlock/BasicVerb.h"
#include "AudioProcessorBlock/Preamp.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //== Tree States ===============================================================
    juce::AudioProcessorValueTreeState treeState;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor);

    //==============================================================================
    AudioProcessorBlock::Preamp preamp;
    AudioProcessorBlock::ThreeTapDelay tapsDelay;
    AudioProcessorBlock::BasicVerb diffuser1stStage, diffuser2stStage;
    juce::AudioBuffer<float> diffuser2stStageBuffer, delayedBuffer;

    void updatePreampParams();
    void updateTapsDelayParams();
    void updateBasicVerbParams();
    void updateOutputParams();

    juce::dsp::DryWetMixer<float> dryWetMixer, decayAmountMixer;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowCutFilter;

    double lastSampleRate;
    float dryWetProportion{ 0.f }, lowCutFrequency{ 20.f }, outGain{ 1.f };
};
