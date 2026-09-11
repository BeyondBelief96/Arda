#include <vg/core/Geodetic2D.h>

bool vg::core::Geodetic2D::Equals(const Geodetic2D &other) const
{
    return m_latitude == other.m_latitude && m_longitude == other.m_longitude;
}

bool vg::core::Geodetic2D::EqualsEpsilon(const Geodetic2D &other, double epsilon)
{
    return std::abs(m_latitude - other.m_latitude) < epsilon && std::abs(m_longitude - other.m_longitude) < epsilon;
}

bool vg::core::Geodetic2D::operator==(const Geodetic2D &other) const
{
    return Equals(other);
}

bool vg::core::Geodetic2D::operator!=(const Geodetic2D &other) const
{
    return !Equals(other);
}
