#pragma once

#include "Utils/Sine.h"

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace Utils {
    class AllPass
    {
    public:
        AllPass() : sampleRate{ 44100.f }, feedback{ 0.56f }, isModulated{ false }
        {};

        ~AllPass()
        {};

        void prepare(const juce::dsp::ProcessSpec &spec, double _sampleRate, int maxDelayInSamples)
        {
            delay.resize(spec.numChannels);
            for (auto& d : delay)
            {
                d.reset();
                d.prepare(spec);
                d.setMaximumDelayInSamples(maxDelayInSamples);
                d.setDelay(1);
            }

            modOsc.resize(spec.numChannels);
            for (auto& o : modOsc)
                o.prepare(_sampleRate);

            modAmount = 20;
            modFreq = 1;
            sampleRate = _sampleRate;
        };

        void process(juce::AudioSampleBuffer &buffer, int channel)
        {
            auto* inputSamples = buffer.getReadPointer(channel);
            auto* outputSamples = buffer.getWritePointer(channel);
            auto numSamples = buffer.getNumSamples();
            float m = .0f;

            for (int s = 0; s < numSamples; ++s)
            {
                if (isModulated)
                    m = modOsc[channel].getNextSample() * modAmount;

                if (channel == 1)
                    m *= -1;
            
                float delayedSample = delay[channel].popSample(channel, apSampleDelay + m);
                float sampleToDelay = inputSamples[s] + (-feedback * delayedSample);
                delay[channel].pushSample(channel, sampleToDelay);
                outputSamples[s] = delayedSample + (feedback * sampleToDelay);
            }
        };

        void setAPSampleDelay(float apDelay)
        {
            if (apDelay != apSampleDelay)
            {
                apSampleDelay = apDelay;
                for (auto& d : delay)
                    d.setDelay(apSampleDelay);
            }
        };

        void setFeedback(float _feedback)
        {
            feedback = _feedback;
        };

        void setModFreq(float freq)
        {
            if (freq != modFreq)
            {
                modFreq = freq;
                for (auto& o : modOsc)
                    o.setFrequency(modFreq);
            }
        };

        void setModAmount(float amount)
        {
            if (amount != modAmount)
                modAmount = amount;
        };

        void setModulation(bool isOn)
        {
            if (isOn != isModulated)
                isModulated = isOn;
        };

    private:
        bool isModulated;
        double sampleRate;
        float feedback, apSampleDelay, modFreq, modAmount;

        std::vector<Utils::Sine> modOsc;
        std::vector<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>> delay;
    };
}