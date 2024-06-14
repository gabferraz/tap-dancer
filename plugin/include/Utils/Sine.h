#pragma once

#include <cmath>

#define M_PI 3.14159265359f

namespace Utils
{
    class Sine
    {
    private:
        float period { .0f }, frequency { .0f }, sampleRate;
        float currentAngle { .0f }, angleDelta { .0f };

        void updateAngle()
        {
            currentAngle += angleDelta;
            if (currentAngle >= 2 * M_PI)
                currentAngle -= 2 * M_PI;
        }

    public:
        Sine(/* args */)
        {};

        ~Sine()
        {};

        void prepare (double _sampleRate)
        {
            sampleRate = static_cast<float>(_sampleRate);
        }

        float getNextSample()
        {
            float sample = std::sin(currentAngle);
            updateAngle();
            return sample;
        }

        void setFrequency(float freq)
        {
            auto cyclesPerSample = freq / sampleRate;
            angleDelta = 2 * M_PI * cyclesPerSample;
        }
    };
}