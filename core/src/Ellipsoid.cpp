#include <vg/core/Ellipsoid.h>

#include <cmath>

namespace vg::core {
    Ellipsoid::Ellipsoid(const Vector3<double>& radii) : m_radii(radii) {
        m_radiiSquared = radii.MultiplyComponentWise(radii);
        m_radiiToTheFourth = m_radiiSquared.MultiplyComponentWise(m_radiiSquared);
        m_oneOverRadiiSquared = Vector3<double>(1.0 / m_radiiSquared.X(), 1.0 / m_radiiSquared.Y(), 1.0 / m_radiiSquared.Z());
    }

    Ellipsoid::Ellipsoid(double radiusX, double radiusY, double radiusZ) : Ellipsoid(Vector3<double>(radiusX, radiusY, radiusZ)) {}

    Ellipsoid Ellipsoid::WGS84() {
        return Ellipsoid(6378137.0, 6378137.0, 6356752.314245);
    }

    Ellipsoid Ellipsoid::UnitSphere() {
        return Ellipsoid(1.0, 1.0, 1.0);
    }

    Vector3<double> Ellipsoid::GeocentricNormal(const Vector3<double> &position) const
    {
        return position.Normalize();
    }

    Vector3<double> Ellipsoid::GeodeticSurfaceNormal(const Vector3<double> &positionOnEllipsoid) const
    {
        return positionOnEllipsoid.MultiplyComponentWise(m_oneOverRadiiSquared).Normalize();
    }

    Vector3<double> Ellipsoid::GeodeticSurfaceNormal(const Geodetic3D &geodeticPosition) const
    {
        const double cosLatitude = std::cos(geodeticPosition.Latitude());
        return Vector3<double>(cosLatitude * std::cos(geodeticPosition.Longitude()), cosLatitude * std::sin(geodeticPosition.Longitude()), std::sin(geodeticPosition.Latitude()));
    }

    Vector3<double> Ellipsoid::GeodeticSurfaceNormal(const Geodetic2D &geodeticSurfacePosition) const
    {
        return GeodeticSurfaceNormal(Geodetic3D(geodeticSurfacePosition.Latitude(), geodeticSurfacePosition.Longitude(), 0.0));
    }

    Vector3<double> Ellipsoid::ToCartesian(const Geodetic3D& geodeticPosition) const
    {
        const Vector3<double> n_s = GeodeticSurfaceNormal(geodeticPosition);
        const Vector3<double> k = m_radiiSquared.MultiplyComponentWise(n_s);
        const double gamma = std::sqrt(k.Dot(n_s));
        return Vector3<double>(k.X() / gamma, k.Y() / gamma, k.Z() / gamma) + n_s * geodeticPosition.Altitude();
    }

    Vector3<double> Ellipsoid::ToCartesian(const Geodetic2D &geodeticSurfacePosition) const
    {
        return ToCartesian(Geodetic3D(geodeticSurfacePosition.Latitude(), geodeticSurfacePosition.Longitude(), 0.0));
    }

    Geodetic2D Ellipsoid::ToGeodetic2D(const Vector3<double> &cartesianSurfacePosition) const
    {
        const Vector3<double> n_s = GeodeticSurfaceNormal(cartesianSurfacePosition);
        const double latitude = std::asin(n_s.Z() / n_s.Magnitude());
        const double longitude = std::atan2(n_s.Y(), n_s.X());
        return Geodetic2D(latitude, longitude);
    }

    Geodetic3D Ellipsoid::ToGeodetic3D(const Vector3<double> &cartesianPosition) const
    {
        Vector3<double> surfacePosition = ScaleToGeodeticSurface(cartesianPosition);
        Vector3<double> h = cartesianPosition - surfacePosition;
        const double height = (h.Dot(cartesianPosition) < 0.0 ? -1.0 : 1.0) * h.Magnitude();
        return Geodetic3D {ToGeodetic2D(surfacePosition), height};
    }

    Vector3<double> Ellipsoid::ScaleToGeocentricSurface(const Vector3<double> &position) const
    {
        const Vector3<double> denominatorComponents = m_oneOverRadiiSquared.MultiplyComponentWise(position);
        const double denominator = std::sqrt(denominatorComponents.Dot(position));
        const double beta = 1.0 / denominator;
        return beta * position;
    }

    Vector3<double> Ellipsoid::ScaleToGeodeticSurface(const Vector3<double> &position) const
    {
        const double beta = 1.0 / std::sqrt(
            (position.X() * position.X()) * m_oneOverRadiiSquared.X() +
            (position.Y() * position.Y()) * m_oneOverRadiiSquared.Y() +
            (position.Z() * position.Z()) * m_oneOverRadiiSquared.Z());

        const double n_s = Vector3<double> {
            beta * position.X() * m_oneOverRadiiSquared.X(),
            beta * position.Y() * m_oneOverRadiiSquared.Y(),
            beta * position.Z() * m_oneOverRadiiSquared.Z()}.Magnitude();

        double alpha = (1.0 - beta) * (position.Magnitude() / n_s);

        const double x_squared = position.X() * position.X();
        const double y_squared = position.Y() * position.Y();
        const double z_squared = position.Z() * position.Z();

        double da = 0.0;
        double db = 0.0;
        double dc = 0.0;

        double s = 0.0;
        double dSdA = 1.0;

        do
        {
            alpha -= (s / dSdA);

            da = 1.0 + (alpha * m_oneOverRadiiSquared.X());
            db = 1.0 + (alpha * m_oneOverRadiiSquared.Y());
            dc = 1.0 + (alpha * m_oneOverRadiiSquared.Z());

            const double da_squared = da * da;
            const double db_squared = db * db;
            const double dc_squared = dc * dc;

            const double da_cubed = da * da_squared;
            const double db_cubed = db * db_squared;
            const double dc_cubed = dc * dc_squared;

            s = x_squared / (m_radiiSquared.X() * da_squared) + y_squared / (m_radiiSquared.Y() * db_squared) + z_squared / (m_radiiSquared.Z() * dc_squared) - 1.0;
            dSdA = -2.0 * (x_squared / (m_radiiToTheFourth.X() * da_cubed) + y_squared / (m_radiiToTheFourth.Y() * db_cubed) + z_squared / (m_radiiToTheFourth.Z() * dc_cubed));
        } while (std::abs(s) > 1e-10);

        return Vector3<double> {position.X() / da, position.Y() / db, position.Z() / dc};
    }
}
