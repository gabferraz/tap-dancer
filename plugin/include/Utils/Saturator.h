#pragma once

#include <cmath>

namespace Utils {
    template <typename T>
    T saturate(T in)
    {
        auto s = pow(in, 3.f) + exp(in) - 1;
        return tanh(s);
    }
}