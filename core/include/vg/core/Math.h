#pragma once

#include <algorithm>
#include <cmath>

namespace vg::core {
    class Math {
        public:
            static constexpr double Epsilon6 = 1e-6;
            static constexpr double Epsilon10 = 1e-10;
            static constexpr double Epsilon14 = 1e-14;

            static bool NearlyZero(double value, double epsilon) {
                return std::abs(value) <= epsilon;
            }

            static bool NearlyEqual(double a, double b, double epsilon) {
                return std::abs(a - b) <= epsilon * std::max({1.0, std::abs(a), std::abs(b)});
            }
    };
}
