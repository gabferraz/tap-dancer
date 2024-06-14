#pragma once

#include "Utils/Allpass.h"

namespace AudioProcessorBlock
{
    class BasicVerb
    {
    private:
        Utils::AllPass ap1, ap2, apMod, apFeedback;

        std::vector<juce::dsp::IIR::Filter<float>> outputLowPass;
        juce::dsp::IIR::Coefficients<float>::Ptr outputFilterCoefficients;

        std::vector<juce::dsp::IIR::Filter<float>> dampingLowPass;
        juce::dsp::IIR::Coefficients<float>::Ptr dampingFilterCoefficients;

        double sampleRate;
        float decay, damp, feedback;
        juce::AudioBuffer<float> previousBuffer;

    public:
        BasicVerb()
        {};

        ~BasicVerb()
        {};

        void prepare(const juce::dsp::ProcessSpec &spec, double _sampleRate);
        void process(juce::AudioSampleBuffer &buffer);
        void updateParams(float _decay, float _damp, float modRate, float modAmount);
    };

    inline void BasicVerb::prepare(const juce::dsp::ProcessSpec &spec, double _sampleRate)
    {
        sampleRate = _sampleRate;

        // Prepare all pass filters
        ap1.prepare(spec, sampleRate, 6500);
        ap2.prepare(spec, sampleRate, 6500);
        apMod.prepare(spec, sampleRate, 6500);
        apMod.setModulation(true);
        apFeedback.prepare(spec, sampleRate, 6500);

        // Prepare low pass filters
        outputLowPass.resize(spec.numChannels);
        for(auto& f : outputLowPass)
        {
            f.prepare(spec);
            f.reset();
        }
        outputFilterCoefficients = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, 20000.f);

        dampingLowPass.resize(spec.numChannels);
        for(auto& f: dampingLowPass)
        {
            f.prepare(spec);
            f.reset();
        }
        dampingFilterCoefficients = *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, 20000.f);
    }

    inline void BasicVerb::process(juce::AudioSampleBuffer &buffer)
    {
        int numChannels = buffer.getNumChannels();
        for (int channel = 0; channel < numChannels; ++channel)
        {
            if (previousBuffer.getNumSamples() > 0)
            {
                buffer.addFromWithRamp(
                    channel, 
                    0, 
                    previousBuffer.getReadPointer(channel),
                    buffer.getNumSamples(),
                    feedback,
                    feedback
                );
            }

            // Serial all pass filter stage
            ap1.process(buffer, channel);
            ap2.process(buffer, channel);

            // Feedback all pass filter stage
            previousBuffer.copyFrom(
                channel,
                0,
                buffer.getReadPointer(channel),
                previousBuffer.getNumSamples()
            );
            apFeedback.process(previousBuffer, channel);

            auto* pbWriteHead = previousBuffer.getWritePointer(channel);
            auto* pbReadHead = previousBuffer.getReadPointer(channel);
            for (int s = 0; s < previousBuffer.getNumSamples(); ++s)
            {
                auto sample = dampingLowPass[channel].processSample(pbReadHead[s]);
                pbWriteHead[s] = static_cast<float>(std::tanh(-sample));
            }

            // Process output stage
            apMod.process(buffer, channel);

            auto* outWriteHead = buffer.getWritePointer(channel);
            auto* outReadHead = buffer.getReadPointer(channel);
            for (int s = 0; s < buffer.getNumSamples(); ++s)
                outWriteHead[s] = outputLowPass[channel].processSample(outReadHead[s]);
        }
    }

    inline void BasicVerb::updateParams(float _decay, float _damp, float modRate, float modAmount)
    {
        if (_decay != decay)
        {  
            decay = _decay;
            feedback = decay / 4000.f;

            ap1.setAPSampleDelay(decay);
            ap2.setAPSampleDelay(decay * 1.39f);
            apMod.setAPSampleDelay(decay * 1.93f);
            apFeedback.setAPSampleDelay(decay * 2.f);
            apFeedback.setFeedback(.6f);
        }
        
        if (_damp != damp)
        {
            damp = _damp;
            outputFilterCoefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, _damp);
            for (auto& f : outputLowPass)
                f.coefficients = outputFilterCoefficients;

            dampingFilterCoefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, _damp / 1.4f);
            for (auto& f : dampingLowPass)
                f.coefficients = dampingFilterCoefficients;
        }

        apMod.setModAmount(modAmount);
        apMod.setModFreq(modRate);
    }
}