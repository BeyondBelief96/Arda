#include <arda/core/Trig.h>

namespace arda::core {

    double Trig::DegreesToRadians(double degrees)
    {
        return degrees * RadiansPerDegree;
    }

    Geodetic2D Trig::DegreesToRadians(const Geodetic2D &degrees)
    {
        return Geodetic2D(DegreesToRadians(degrees.Latitude()), DegreesToRadians(degrees.Longitude()));
    }

    Geodetic3D Trig::DegreesToRadians(const Geodetic3D &degrees)
    {
        return Geodetic3D(DegreesToRadians(degrees.Latitude()), DegreesToRadians(degrees.Longitude()), DegreesToRadians(degrees.Altitude()));
    }

    double Trig::ToDegrees(double radians)
    {
        return radians * (180.0 / std::numbers::pi);
    }

    Geodetic2D Trig::ToDegrees(const Geodetic2D &radians)
    {
        return Geodetic2D(ToDegrees(radians.Latitude()), ToDegrees(radians.Longitude()));
    }

    Geodetic3D Trig::ToDegrees(const Geodetic3D &radians)
    {
        return Geodetic3D(ToDegrees(radians.Latitude()), ToDegrees(radians.Longitude()), ToDegrees(radians.Altitude()));
    }
}
