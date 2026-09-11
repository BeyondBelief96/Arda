#pragma once

#include <numbers>
#include <vg/core/Geodetic2D.h>
#include <vg/core/Geodetic3D.h>

namespace vg::core {

class Trig {
public:
    inline static constexpr double OneOverPi = 1.0 / std::numbers::pi;
    inline static constexpr double OneOverTwoPi = 1.0 / (2.0 * std::numbers::pi);
    inline static constexpr double OneOverFourPi = 1.0 / (4.0 * std::numbers::pi);
    inline static constexpr double PiOverTwo = std::numbers::pi / 2.0;
    inline static constexpr double PiOverThree = std::numbers::pi / 3.0;
    inline static constexpr double PiOverFour = std::numbers::pi / 4.0;
    inline static constexpr double PiOverSix = std::numbers::pi / 6.0;
    inline static constexpr double TwoPi = 2.0 * std::numbers::pi;
    inline static constexpr double ThreePiOverTwo = 3.0 * std::numbers::pi / 2.0;
    inline static constexpr double RadiansPerDegree = std::numbers::pi / 180.0;

    static double DegreesToRadians(double degrees);
    static Geodetic2D DegreesToRadians(const Geodetic2D &degrees);
    static Geodetic3D DegreesToRadians(const Geodetic3D &degrees);

    static double ToDegrees(double radians);
    static Geodetic2D ToDegrees(const Geodetic2D &radians);
    static Geodetic3D ToDegrees(const Geodetic3D &radians);
};


} // namespace vg::core
