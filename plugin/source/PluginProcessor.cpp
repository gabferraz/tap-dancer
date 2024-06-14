#include "TapDancer/PluginProcessor.h"
#include "TapDancer/PluginEditor.h"

#define JucePlugin_Name "TapDancer"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        treeState(*this, nullptr, "PARAMS", createParameterLayout()),
        lowCutFilter(juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(44100, 20.f))
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // PREAMP SECTION PARAMS
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SATURATE_ID", "Saturate", .0f, 2.f, 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TONE_ID", "Tone", 800.f, 20000.f, 20000.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN_ID", "Gain", .0f, 2.f, 1.f));

    // SPACE SECTION PARAMS
    // Multi Taps Params
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TAPS_ID", "Taps", 0.f, 3.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("FEEDBACK_ID", "Feedback", 0.f, .9f, 0.f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("TAP1F_ID", "Tap 1 Feedback", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("TAP2F_ID", "Tap 2 Feedback", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("TAP3F_ID", "Tap 3 Feedback", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("WIDTH_ID", "Taps Width", 0.f, 1.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TIME_ID", "Delay Time", 50.f, 1000.f, 250.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TSPREAD_ID", "Delay Spread", 0.f, 1000.f, 0.f));

    // Diffuser Params
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIFFUSER_ID", "Diffuser", 0.f, 1.f, 0.f));

    // Taps and Diffuser Params
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MOD_ID", "Modulation", 0.f, 1.f, 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DAMP_ID", "Damping", 2000.f, 20000.f, 20000.f));

    // OUTPUT SECTION PARAMS
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LOWCUT_ID", "Low Cut", 20.f, 1000.f, 20.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DRYWET_ID", "Dry Wet", .0f, 1.f, .5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTPUT_ID", "Output Gain", .0f, 2.f, 1.f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    lastSampleRate = sampleRate;

    // Prepara estágio de preamp
    preamp.prepare(spec);

    // Prepara delay multi-tap
    tapsDelay.prepare(spec, sampleRate);

    // Prepara difusor
    decayAmountMixer.reset();
    decayAmountMixer.prepare(spec);
    decayAmountMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);
    decayAmountMixer.setWetMixProportion(.0f);

    diffuser1stStage.prepare(spec, sampleRate);
    diffuser2stStage.prepare(spec, sampleRate);

    // Prepara controles de saída
    dryWetMixer.reset();
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);

    lowCutFilter.reset();
    lowCutFilter.prepare(spec);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::updatePreampParams()
{
    float saturation = *treeState.getRawParameterValue("SATURATE_ID");
    preamp.setSaturation(saturation);

    float tone = *treeState.getRawParameterValue("TONE_ID");
    preamp.setToneFrequency(tone);

    float gain = *treeState.getRawParameterValue("GAIN_ID");
    preamp.setOutputGain(gain);
}

void AudioPluginAudioProcessor::updateTapsDelayParams()
{
    float taps = *treeState.getRawParameterValue("TAPS_ID");
    tapsDelay.setDelayTaps(taps);

    float feedback = *treeState.getRawParameterValue("FEEDBACK_ID");
    tapsDelay.setDelayFeedback(feedback);

    bool t1HasFeedback = *treeState.getRawParameterValue("TAP1F_ID") > .5f;
    bool t2HasFeedback = *treeState.getRawParameterValue("TAP2F_ID") > .5f;
    bool t3HasFeedback = *treeState.getRawParameterValue("TAP3F_ID") > .5f;
    tapsDelay.setTapsFeeback(t1HasFeedback, t2HasFeedback, t3HasFeedback);

    float width = *treeState.getRawParameterValue("WIDTH_ID");
    tapsDelay.setDelayPanWidth(width);

    float time = *treeState.getRawParameterValue("TIME_ID");
    tapsDelay.setDelayTime(time);

    float spread = *treeState.getRawParameterValue("TSPREAD_ID");
    tapsDelay.setDelaySpread(spread);

    float modulation = *treeState.getRawParameterValue("MOD_ID");
    tapsDelay.setTapsModulation(modulation * 1.5f, modulation * 100.f);

    float damp = *treeState.getRawParameterValue("DAMP_ID");
    tapsDelay.setTapsDamping(damp);
}

void AudioPluginAudioProcessor::updateBasicVerbParams()
{
    float decay = *treeState.getRawParameterValue("DIFFUSER_ID");
    decayAmountMixer.setWetMixProportion(static_cast<float>(std::tanh(decay * 1.5f)));

    float decayTransposed = (decay * 1200.f) + 600.f;
    float modulation = *treeState.getRawParameterValue("MOD_ID");
    float damp = *treeState.getRawParameterValue("DAMP_ID");

    diffuser1stStage.updateParams(decayTransposed, damp, modulation * 1.4f, modulation * 40.f);
    diffuser2stStage.updateParams(decayTransposed, damp, modulation * 1.4f, modulation * -40.f);
}

void AudioPluginAudioProcessor::updateOutputParams()
{
    float dryWetMix = *treeState.getRawParameterValue("DRYWET_ID");
    if (dryWetMix != dryWetProportion)
    {
        dryWetProportion = dryWetMix;
        dryWetMixer.setWetMixProportion(dryWetMix);
    }

    float lowCutFreq = *treeState.getRawParameterValue("LOWCUT_ID");
    if (lowCutFreq != lowCutFrequency)
    {
        lowCutFrequency = lowCutFreq;
        *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(lastSampleRate, lowCutFrequency);
    }

    float outputGain = *treeState.getRawParameterValue("OUTPUT_ID");
    if (outputGain != outGain)
        outGain = outputGain;
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Preamp Stage
    updatePreampParams();
    preamp.process(buffer);

    // Multi Tap Delay Stage
    updateTapsDelayParams();
    tapsDelay.process(buffer);

    // Diffusion Stage
    float diffusion = *treeState.getRawParameterValue("DIFFUSER_ID");
    if (diffusion > 0)
    {
        updateBasicVerbParams();
        decayAmountMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));
        if (diffuser2stStageBuffer.getNumSamples() > 0)
        {
            for (int channel = 0; channel < totalNumOutputChannels; ++channel)
                buffer.addFromWithRamp(channel, 0, diffuser2stStageBuffer.getReadPointer(channel), numSamples, .6f, .6f);
        }
        diffuser1stStage.process(buffer);
        diffuser2stStageBuffer.makeCopyOf(buffer);
        diffuser2stStage.process(diffuser2stStageBuffer);
        decayAmountMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
    }

    updateOutputParams();
    juce::dsp::AudioBlock<float> block(buffer);
    lowCutFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
    dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
    buffer.applyGain(outGain);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
