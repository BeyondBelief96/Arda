#include <vg/core/Geodetic3D.h>

bool vg::core::Geodetic3D::Equals(const Geodetic3D &other) const
{
    return latitude == other.latitude && longitude == other.longitude && altitude == other.altitude;
}

bool vg::core::Geodetic3D::EqualsEpsilon(const Geodetic3D &other, double epsilon) const
{
    return std::abs(latitude - other.latitude) < epsilon &&
           std::abs(longitude - other.longitude) < epsilon &&
           std::abs(altitude - other.altitude) < epsilon;
}

bool vg::core::Geodetic3D::operator==(const Geodetic3D &other) const
{
    return Equals(other);
}

bool vg::core::Geodetic3D::operator!=(const Geodetic3D &other) const
{
    return !Equals(other);
}
