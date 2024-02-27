#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace utils
{
    template <typename T>
    T msToSamples(T delayInMs, double sampleRate)
    {
        return sampleRate * delayInMs * .001f;
    }

    class Delay
    {
    private:
        juce::AudioBuffer<float> buffer;
        juce::AudioBuffer<float> delayedBlock;
        int writePosition { 0 };
        int delayBufferSize { 0 };
        int delayTime { 0 };
        double sampleRate { .0f };

        //====================================================================================================
        void writeToDelayBuffer(juce::AudioBuffer<float>& sourceBuffer, int channel)
        {
            int bufferSize = sourceBuffer.getNumSamples();
            float* bufferWritePointer = sourceBuffer.getWritePointer(channel);

            // Preenche buffer do delay com a lógica de um buffer circular
            if (delayBufferSize > (bufferSize + writePosition))
            {
                buffer.copyFrom(channel, writePosition, bufferWritePointer, bufferSize);
            } else {
                auto freeSamplesInBuffer = delayBufferSize - writePosition;
                buffer.copyFrom(channel, writePosition, bufferWritePointer, freeSamplesInBuffer);

                auto numSamplesToWrapAround = bufferSize - freeSamplesInBuffer;
                buffer.copyFrom(channel, 0, bufferWritePointer + freeSamplesInBuffer, numSamplesToWrapAround);
            }
        }

        //====================================================================================================
        void readFromDelayedBuffer(juce::AudioBuffer<float>& sourceBuffer, int channel)
        {
            int bufferSize = sourceBuffer.getNumSamples();
            int numChannels = sourceBuffer.getNumChannels();
            delayedBlock.setSize(numChannels, bufferSize);

            // Lê do buffer com um atraso definido
            auto readPosition = writePosition - delayTime;
            if (readPosition < 0)
                readPosition += delayBufferSize;

            // Check
            if (readPosition + bufferSize < delayBufferSize)
            {
                auto bufferReadPointer = buffer.getReadPointer(channel, readPosition); 
                delayedBlock.copyFrom(channel, 0, bufferReadPointer, bufferSize);
            } else {
                auto bufferReadPointer = buffer.getReadPointer(channel, readPosition); 
                auto remainingSamplesInBuffer = delayBufferSize - readPosition;
                delayedBlock.copyFrom(channel, 0, bufferReadPointer, remainingSamplesInBuffer);

                auto wrappingSamplesInBuffer = bufferSize - remainingSamplesInBuffer;
                bufferReadPointer = buffer.getReadPointer(channel, 0);
                delayedBlock.copyFrom(channel, remainingSamplesInBuffer, bufferReadPointer, wrappingSamplesInBuffer);
            }
        }

    public:
        Delay()
        {};

        ~Delay()
        {};

        //====================================================================================================
        void prepare(double maxDelayTimeInMs, int numChannels, double _sampleRate)
        {
            delayBufferSize = static_cast<int>(msToSamples(maxDelayTimeInMs, _sampleRate));
            buffer.setSize(numChannels, delayBufferSize);
            sampleRate = _sampleRate;
        };

        //====================================================================================================
        void processBlock(juce::AudioBuffer<float>& sourceBuffer, int channel)
        {
            writeToDelayBuffer(sourceBuffer, channel);
            readFromDelayedBuffer(sourceBuffer, channel);
        };

        //====================================================================================================
        void setWritePosition(int bufferSize)
        {
            writePosition += bufferSize;
            writePosition %= delayBufferSize;
        };

        //====================================================================================================
        void setDelayTime(double delayTimeInMs)
        {
            delayTime = static_cast<int>(msToSamples(delayTimeInMs, sampleRate));
        };

        //====================================================================================================
        juce::AudioBuffer<float> getDelayedSignalBuffer()
        {
            return delayedBlock;
        }
    };   
}