#pragma once

#include <vg/core/Vector3.h>
#include <vg/core/Geodetic3D.h>
#include <vg/core/Geodetic2D.h>
namespace vg::core {
    class Ellipsoid {
        public:
            Ellipsoid() = default;
            Ellipsoid(const Vector3<double>& radii);
            Ellipsoid(double radiusX, double radiusY, double radiusZ);

            static Ellipsoid WGS84();
            static Ellipsoid UnitSphere();

            double GetSemiMajorAxis() const { return m_radii.X(); }
            double GetSemiMinorAxis() const { return m_radii.Z(); }
            Vector3<double> GetRadii() const { return m_radii;}
            Vector3<double> GetRadiiSquared() const { return m_radiiSquared; }
            Vector3<double> GetOneOverRadiiSquared() const { return m_oneOverRadiiSquared; }
            Vector3<double> GetRadiiToTheFourth() const { return m_radiiToTheFourth; }

            /**
             * Computes the geocentric normal at a position: the unit vector pointing from the ellipsoid's center toward that position.
             * Unlike GeodeticSurfaceNormal, this treats the ellipsoid as a sphere, so the position need not lie on its surface.
             * @param position The position to compute the geocentric normal for.
             * @return A unit vector pointing from the ellipsoid's center toward the given position.
             */
            Vector3<double> GeocentricNormal(const Vector3<double>& position) const;

            /**
             * Computes the surface normal of the ellipsoid at a position assumed to lie on its surface,
             * i.e. the outward-pointing gradient of the ellipsoid's implicit surface equation at that position.
             * @param positionOnEllipsoid A position on the surface of the ellipsoid.
             * @return A unit vector pointing in the direction of the geodetic surface normal at the given position.
             */
            Vector3<double> GeodeticSurfaceNormal(const Vector3<double>& positionOnEllipsoid) const;

            /**
             * Computes the surface normal of the ellipsoid at a geodetic position, using only its latitude
             * and longitude (altitude does not affect the normal's direction).
             * @param geodeticPosition The geodetic position.
             * @return A unit vector pointing in the direction of the geodetic surface normal at the given position.
             */
            Vector3<double> GeodeticSurfaceNormal(const Geodetic3D& geodeticPosition) const;

            /**
             * Computes the surface normal of the ellipsoid at a geodetic surface position (latitude and longitude).
             * @param geodeticSurfacePosition The geodetic position on the surface of the ellipsoid.
             * @return A unit vector pointing in the direction of the geodetic surface normal at the given position.
             */
            Vector3<double> GeodeticSurfaceNormal(const Geodetic2D& geodeticSurfacePosition) const;

            /**
             * Converts a geodetic position (latitude, longitude, altitude) to a cartesian position on this ellipsoid.
             * @param geodeticPosition The geodetic position.
             * @return The cartesian position with reference to this ellipsoid.
             */
            Vector3<double> ToCartesian(const Geodetic3D& geodeticPosition) const;

            /**
             * Converts a geodetic surface position (latitude, longitude, zero altitude) to a cartesian position on this ellipsoid.
             * @param geodeticSurfacePosition The geodetic position on the surface of the ellipsoid.
             * @return The cartesian position with reference to this ellipsoid.
             */
            Vector3<double> ToCartesian(const Geodetic2D& geodeticSurfacePosition) const;

            /**
             * Converts a cartesian position on the surface of the ellipsoid to a geodetic position.
             * @param cartesianSurfacePosition The cartesian position on the surface of the ellipsoid.
             * @return The geodetic position.
             */
            Geodetic2D ToGeodetic2D(const Vector3<double>& cartesianSurfacePosition) const;

            /**
             * Converts a cartesian position in WGS84 coordinates to a geodetic position.
             * @param cartesianPosition The cartesian position to convert.
             * @return The geodetic position.
             */
            Geodetic3D ToGeodetic3D(const Vector3<double>& cartesianPosition) const;

            /**
             * Scales a position to the surface of the ellipsoid along the geocentric surface normal.
             * @param position The position to scale.
             * @return The scaled position on the surface of the ellipsoid.
             */
            Vector3<double> ScaleToGeocentricSurface(const Vector3<double>& position) const;

            /**
             * Scales an arbitrary position in cartesian coordinates to the surface of the ellipsoid
             * along the direction of the geodetic surface normal to that point from the surface using
             * the Newton-Raphson iterative method.
             * @param position The position to scale.
             * @return The scaled position on the surface of the ellipsoid.
             */
            Vector3<double> ScaleToGeodeticSurface(const Vector3<double>& position) const;
        private:
            Vector3<double> m_radii;
            Vector3<double> m_radiiSquared;
            Vector3<double> m_radiiToTheFourth;
            Vector3<double> m_oneOverRadiiSquared;
    };
}
