#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Utils/Sine.h"

namespace Utils {
    class Delay
    {
    public:
        Delay() : sampleRate{ 44100.f }, feedback{ .0f }
        {};

        ~Delay()
        {};

        void prepare(const juce::dsp::ProcessSpec &spec, double _sampleRate, float maxDelayInMs)
        {
            auto maxDelayInSamples = static_cast<int>(msToSamples(maxDelayInMs));

            delay.resize(spec.numChannels);
            for (auto& d : delay)
            {
                d.reset();
                d.prepare(spec);
                d.setMaximumDelayInSamples(maxDelayInSamples);
                d.setDelay(1);
            }

            filterCoef = juce::dsp::FilterDesign<float>::designFIRLowpassWindowMethod(20000.f, _sampleRate, 21, juce::dsp::WindowingFunction<float>::hamming);
            dampFilter.resize(spec.numChannels);
            for (auto& f : dampFilter)
            {
                f.reset();
                f.prepare(spec);
                f.coefficients = filterCoef;
            }

            modOsc.resize(spec.numChannels);
            for (auto& osc : modOsc)
                osc.prepare(sampleRate);

            modAmount = 20;
            modFreq = 50;
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
                if (modAmount > 0)
                    m = (modOsc[channel].getNextSample() + 1) * modAmount;
            
                float delayedSample = delay[channel].popSample(channel, time + m);
                float sampleToDelay = inputSamples[s] + dampFilter[channel].processSample(std::tanh((feedback * delayedSample)));
                delay[channel].pushSample(channel, sampleToDelay);
                outputSamples[s] = delayedSample;
            }
        };

        void setDelayTime(float delayTimeInMs)
        {
            auto delayTime = msToSamples(delayTimeInMs);
            time = delayTime;
            for (auto& d : delay)
                d.setDelay(time);

        };

        void setFeedback(float _feedback)
        {
            feedback = _feedback;
        };

        void setModFreq(float freq)
        {
            modFreq = freq;
            for (auto& osc : modOsc)
                osc.setFrequency(modFreq);
        };

        void setModAmount(float amount)
        {
            modAmount = amount;
        };

        void setDamp(float freq)
        {
            filterCoef = juce::dsp::FilterDesign<float>::designFIRLowpassWindowMethod(freq, sampleRate, 21, juce::dsp::WindowingFunction<float>::hamming);
            for(auto& f : dampFilter)
                f.coefficients = filterCoef;
        };

    private:
        double sampleRate;
        float feedback, time;
        float modFreq, modAmount, modPhase;
        std::vector<Utils::Sine> modOsc;
        std::vector<juce::dsp::FIR::Filter<float>> dampFilter;
        juce::dsp::FIR::Coefficients<float>::Ptr filterCoef;
        std::vector<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>> delay;

        float msToSamples(float timeInMs) {
            return static_cast<float>(sampleRate) * timeInMs * 0.001f;
        }
    };
}