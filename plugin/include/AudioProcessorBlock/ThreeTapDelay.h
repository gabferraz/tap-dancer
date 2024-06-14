#pragma once

#include "Utils/Delay.h"

namespace AudioProcessorBlock
{
    class ThreeTapDelay
    {
    private:
        std::vector<juce::dsp::Panner<float>> tapPan;
        std::vector<juce::AudioBuffer<float>> tapBuffer;

        std::vector<std::unique_ptr<Utils::Delay>> tap;
        std::vector<float> tapVolume;
        int numberOfTaps{ 3 };
        float delayTime{ 1.f }, delaySpread{ 0.f }, delayPanWidth{ 0.f }, delayTaps{ 0.f }, delayFeedback{ 0.f };
        float modFrequency{ 0.f }, modAmount{ 0.f }, tapDamping{ 20000.f };
        bool tap1Feedback{ false }, tap2Feedback{ false }, tap3Feedback{ false };

    public:
        ThreeTapDelay()
        {};

        ThreeTapDelay(int numOfTaps) 
        {numberOfTaps = numOfTaps;};

        ~ThreeTapDelay()
        {};

        void prepare(const juce::dsp::ProcessSpec &spec, double _sampleRate);
        void process(juce::AudioBuffer<float>& buffer);

        void setDelayTime(float time);
        void setDelaySpread(float spread);
        void setDelayPanWidth(float width);
        void setDelayTaps(float taps);
        void setDelayFeedback(float feedback);
        void setTapsFeeback(bool t1Feedback, bool t2Feedback, bool t3Feeback);
        void setTapsModulation(float freq, float amount);
        void setTapsDamping(float freq);
    };
    
    //========================================================================================
    inline void ThreeTapDelay::prepare(const juce::dsp::ProcessSpec &spec, double _sampleRate)
    {
        tap.resize(numberOfTaps);
        for(auto& t : tap)
        {
            auto tapPtr = std::make_unique<Utils::Delay>();
            t = std::move(tapPtr);
            t->prepare(spec, _sampleRate, 3500.f);
        }

        tapPan.resize(numberOfTaps);
        for(auto& tp : tapPan)
        {
            tp.reset();
            tp.prepare(spec);
            tp.setRule(juce::dsp::PannerRule::balanced);
            tp.setPan(0.f);
        }

        tapBuffer.resize(numberOfTaps);
        tapVolume.resize(numberOfTaps);
    }

    inline void ThreeTapDelay::process(juce::AudioBuffer<float>& buffer)
    {
        int channels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        // Copy input to each tap buffer
        for(auto& b : tapBuffer) 
            b.makeCopyOf(buffer);
        
        // Process delay taps
        for(int i = 0; i < 3; ++i)
        {
            for(int ch = 0; ch < channels; ++ch)
            {
                if (tapVolume[i] > 0)
                    tap[i]->process(tapBuffer[i], ch);
            }
        }

        // Apply pan to each delay depending on width parameter
        if (delayPanWidth != 0.f)
        {
            for(int i = 0; i < 3; ++i)
            {
                if (tapVolume[i] > 0)
                {
                    juce::dsp::AudioBlock<float> block(tapBuffer[i]);
                    tapPan[i].process(juce::dsp::ProcessContextReplacing<float>(block));
                }
            }
        }

        // Mix all the taps signals on buffer
        for(int i = 0; i < 3; ++i)
        {
            if (i == 0)
            {
                buffer.makeCopyOf(tapBuffer[i]);
                buffer.applyGain(tapVolume[i]);
            } else {
                for(int ch = 0; ch < channels; ++ch)
                {
                    if (tapVolume[i] > 0)
                        buffer.addFromWithRamp(ch, 0, tapBuffer[i].getReadPointer(ch), numSamples, tapVolume[i], tapVolume[i]);
                }
            }
        }
    }

    //========================================================================================
    inline void ThreeTapDelay::setDelayTime(float time) {
        if (time != delayTime) 
        {
            delayTime = time;

            tap[0]->setDelayTime(delayTime);
            tap[1]->setDelayTime(delayTime + delaySpread);
            tap[2]->setDelayTime(delayTime + 2.f * delaySpread);
        }
    }

    inline void ThreeTapDelay::setDelaySpread(float spread) {
        if (spread != delaySpread)
        {
            delaySpread = spread;

            tap[1]->setDelayTime(delayTime + delaySpread);
            tap[2]->setDelayTime(delayTime + 2.f * delaySpread);
        }
    }

    inline void ThreeTapDelay::setDelayPanWidth(float width) {
        if (width != delayPanWidth)
        {
            delayPanWidth = width;

            tapPan[0].setPan(delayPanWidth);
            tapPan[1].setPan(-delayPanWidth);
        }
    }

    inline void ThreeTapDelay::setDelayTaps(float taps) {
        if (taps != delayTaps)
        {
            if (taps <= 1) {
                tapVolume[0] = taps;
                tapVolume[1] = 0.f;
                tapVolume[2] = 0.f;
            }

            if (taps > 1 && taps <= 2) {
                tapVolume[0] = 1.f;
                tapVolume[1] = taps - 1.f;
                tapVolume[2] = 0;
            }

            if (taps > 2) {
                tapVolume[0] = 1.f;
                tapVolume[1] = 1.f;
                tapVolume[2] = taps - 2.f;
            }
        }
    }

    inline void ThreeTapDelay::setDelayFeedback(float feedback) {
        if (feedback != delayFeedback) 
        {
            delayFeedback = feedback;

            if (tap1Feedback)
                tap[0]->setFeedback(delayFeedback);
            if (tap2Feedback)
                tap[1]->setFeedback(delayFeedback);
            if (tap3Feedback)
                tap[2]->setFeedback(delayFeedback);
        }
    }

    inline void ThreeTapDelay::setTapsFeeback(bool t1Feedback, bool t2Feedback, bool t3Feedback) 
    {
        if (tap1Feedback != t1Feedback)
        {
            tap1Feedback = t1Feedback;
            if (tap1Feedback)
                tap[0]->setFeedback(delayFeedback);
            else
                tap[0]->setFeedback(0.f);
        }

        if (tap2Feedback != t2Feedback)
        {
            tap2Feedback = t2Feedback;
            if (tap2Feedback)
                tap[1]->setFeedback(delayFeedback);
            else
                tap[1]->setFeedback(0.f);
        }

        if (tap3Feedback != t3Feedback)
        {
            tap3Feedback = t3Feedback;
            if (tap3Feedback)
                tap[2]->setFeedback(delayFeedback);
            else
                tap[2]->setFeedback(0.f);
        }
    }

    inline void ThreeTapDelay::setTapsModulation(float freq, float amount)
    {
        if (freq != modFrequency)
        {
            modFrequency = freq;
            for (auto& t : tap)
                t->setModFreq(modFrequency);
        }

        if (amount != modAmount)
        {
            modAmount = amount;
            for (auto& t : tap)
                t->setModAmount(modAmount);
        }
    }

    inline void ThreeTapDelay::setTapsDamping(float freq)
    {
        if (freq != tapDamping)
        {
            tapDamping = freq;
            for (auto& t : tap)
                t->setDamp(tapDamping);
        }
    }

}