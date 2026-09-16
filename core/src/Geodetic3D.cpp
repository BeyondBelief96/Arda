#include <arda/core/Geodetic3D.h>

namespace arda::core {

    Geodetic3D::Geodetic3D(double latitude, double longitude, double altitude)
        : latitude(latitude), longitude(longitude), altitude(altitude)
    {
    }

    Geodetic3D::Geodetic3D(Geodetic2D surfacePosition, double height)
        : Geodetic3D(surfacePosition.Latitude(), surfacePosition.Longitude(), height)
    {
    }

    bool Geodetic3D::Equals(const Geodetic3D &other) const
    {
        return latitude == other.latitude && longitude == other.longitude && altitude == other.altitude;
    }

    bool Geodetic3D::EqualsEpsilon(const Geodetic3D &other, double epsilon) const
    {
        return std::abs(latitude - other.latitude) < epsilon &&
            std::abs(longitude - other.longitude) < epsilon &&
            std::abs(altitude - other.altitude) < epsilon;
    }

    bool Geodetic3D::operator==(const Geodetic3D &other) const
    {
        return Equals(other);
    }

    bool Geodetic3D::operator!=(const Geodetic3D &other) const
    {
        return !Equals(other);
    }
}
