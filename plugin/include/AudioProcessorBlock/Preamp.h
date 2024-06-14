#pragma once

#include "Utils/Saturator.h"

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace AudioProcessorBlock
{
    class Preamp
    {
    private:
        double sampleRate{ 44100.f }; 
        float saturation{ 1.f }, gain{ 1.f }, tone{ 20000.f };
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> toneFilter;

    public:
        Preamp() : toneFilter(juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(44100, 20000.f))
        {};

        ~Preamp()
        {};

        void prepare(juce::dsp::ProcessSpec& spec)
        {
            sampleRate = spec.sampleRate;
            toneFilter.reset();
            toneFilter.prepare(spec);
        };

        void process(juce::AudioBuffer<float>& buffer)
        {
            int channels = buffer.getNumChannels();
            int numSamples = buffer.getNumSamples();

            for (int channel = 0; channel < channels; ++channel)
            {
                auto channelData = buffer.getWritePointer(channel);
                for (int sample = 0; sample < numSamples; ++sample)
                    channelData[sample] = static_cast<float>(Utils::saturate(channelData[sample] * saturation));
            }

            juce::dsp::AudioBlock<float> block(buffer);
            toneFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
            buffer.applyGain(gain);
        };

        void setSaturation(float inGain)
        {
            if (inGain != saturation)
                saturation = inGain;
        };

        void setOutputGain(float outGain)
        {
            if (outGain != gain)
                gain = outGain;
        };

        void setToneFrequency(float freq)
        {
            if (freq != tone)
            {
                tone = freq;
                *toneFilter.state = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, tone);
            }
        };
    };
}