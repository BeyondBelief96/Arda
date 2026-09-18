#include <doctest/doctest.h>

#include <arda/core/Trig.h>
#include <arda/core/geometry/Ellipsoid.h>

#include <cmath>
#include <stdexcept>

using arda::core::Geodetic2D;
using arda::core::Geodetic3D;
using arda::core::Trig;
using arda::core::Vector3;
using arda::core::geometry::Ellipsoid;

namespace {
    void CheckVectorNear(const Vector3<double>& actual, const Vector3<double>& expected, double epsilon) {
        CHECK(actual.X() == doctest::Approx(expected.X()).epsilon(epsilon));
        CHECK(actual.Y() == doctest::Approx(expected.Y()).epsilon(epsilon));
        CHECK(actual.Z() == doctest::Approx(expected.Z()).epsilon(epsilon));
    }

    // Value of the implicit surface equation (x/a)^2 + (y/b)^2 + (z/c)^2; 1 means on the surface.
    double SurfaceEquation(const Ellipsoid& ellipsoid, const Vector3<double>& p) {
        return p.MultiplyComponentWise(p).Dot(ellipsoid.GetOneOverRadiiSquared());
    }
}

TEST_CASE("WGS84 radii") {
    const Ellipsoid wgs84 = Ellipsoid::WGS84();
    CHECK(wgs84.GetSemiMajorAxis() == 6378137.0);
    CHECK(wgs84.GetSemiMinorAxis() == doctest::Approx(6356752.314245));
}

TEST_CASE("ToCartesian at cardinal points") {
    const Ellipsoid wgs84 = Ellipsoid::WGS84();
    const double a = wgs84.GetSemiMajorAxis();
    const double b = wgs84.GetSemiMinorAxis();

    // Absolute epsilon on zero components: Approx is relative, so compare zeros by distance.
    auto near = [](const Vector3<double>& actual, const Vector3<double>& expected) {
        CHECK((actual - expected).Magnitude() < 1e-6);
    };

    near(wgs84.ToCartesian(Geodetic2D(0.0, 0.0)), {a, 0.0, 0.0});
    near(wgs84.ToCartesian(Geodetic2D(0.0, Trig::PiOverTwo)), {0.0, a, 0.0});
    near(wgs84.ToCartesian(Geodetic2D(Trig::PiOverTwo, 0.0)), {0.0, 0.0, b});
    near(wgs84.ToCartesian(Geodetic2D(-Trig::PiOverTwo, 0.0)), {0.0, 0.0, -b});
    near(wgs84.ToCartesian(Geodetic3D(0.0, 0.0, 1000.0)), {a + 1000.0, 0.0, 0.0});
}

TEST_CASE("ToCartesian produces points on the surface") {
    const Ellipsoid wgs84 = Ellipsoid::WGS84();
    for (int lat = -90; lat <= 90; lat += 15) {
        for (int lon = -180; lon <= 180; lon += 30) {
            const Vector3<double> p = wgs84.ToCartesian(Trig::DegreesToRadians(Geodetic2D(lat, lon)));
            CAPTURE(lat);
            CAPTURE(lon);
            CHECK(SurfaceEquation(wgs84, p) == doctest::Approx(1.0).epsilon(1e-12));
        }
    }
}

TEST_CASE("Geodetic surface normal differs from geocentric normal off the equator") {
    const Ellipsoid wgs84 = Ellipsoid::WGS84();
    const Geodetic2D geodetic = Trig::DegreesToRadians(Geodetic2D(45.0, 0.0));
    const Vector3<double> p = wgs84.ToCartesian(geodetic);

    // The normal computed from the cartesian point must match the one from lat/lon.
    CheckVectorNear(wgs84.GeodeticSurfaceNormal(p), wgs84.GeodeticSurfaceNormal(geodetic), 1e-12);

    // On an oblate ellipsoid the geocentric latitude is smaller than the geodetic latitude
    // (by ~0.19 degrees at 45 degrees on WGS84).
    const double geocentricLatitude = std::asin(wgs84.GeocentricNormal(p).Z());
    CHECK(geocentricLatitude < geodetic.Latitude());
    CHECK(Trig::ToDegrees(geodetic.Latitude() - geocentricLatitude) == doctest::Approx(0.1924).epsilon(1e-3));
}

TEST_CASE("Geodetic round trip: ToCartesian then ToGeodetic3D") {
    const Ellipsoid wgs84 = Ellipsoid::WGS84();
    const double heights[] = {-5000.0, 0.0, 1.0, 8848.0, 400000.0};
    for (int lat = -85; lat <= 85; lat += 17) {
        for (int lon = -175; lon <= 175; lon += 35) {
            for (double height : heights) {
                const Geodetic3D original(Trig::DegreesToRadians(lat), Trig::DegreesToRadians(lon), height);
                const Geodetic3D result = wgs84.ToGeodetic3D(wgs84.ToCartesian(original));
                CAPTURE(lat);
                CAPTURE(lon);
                CAPTURE(height);
                CHECK(result.Latitude() == doctest::Approx(original.Latitude()).epsilon(1e-10));
                CHECK(result.Longitude() == doctest::Approx(original.Longitude()).epsilon(1e-10));
                // ScaleToGeodeticSurface stops once |s| < 1e-10, which on WGS84 bounds the
                // height error at roughly R * 1e-10 / 2 ~= 0.3 mm (seen: ~5 um at 1 m heights).
                CHECK(std::abs(result.Altitude() - original.Altitude()) < 1e-3);
            }
        }
    }
}

TEST_CASE("ScaleToGeocentricSurface keeps direction and lands on the surface") {
    const Ellipsoid ellipsoid(1.0, 1.0, 0.7);
    const Vector3<double> p(3.0, -2.0, 5.0);
    const Vector3<double> scaled = ellipsoid.ScaleToGeocentricSurface(p);
    CHECK(SurfaceEquation(ellipsoid, scaled) == doctest::Approx(1.0));
    CheckVectorNear(scaled.Normalize(), p.Normalize(), 1e-12);
}

TEST_CASE("ScaleToGeodeticSurface moves along the surface normal") {
    const Ellipsoid ellipsoid(1.0, 1.0, 0.7);
    const Vector3<double> p(0.8, 0.3, 1.2);
    const Vector3<double> surface = ellipsoid.ScaleToGeodeticSurface(p);
    CHECK(SurfaceEquation(ellipsoid, surface) == doctest::Approx(1.0).epsilon(1e-9));

    // The offset from the surface point to p must be parallel to the normal there.
    const Vector3<double> offset = (p - surface).Normalize();
    CHECK(offset.Cross(ellipsoid.GeodeticSurfaceNormal(surface)).Magnitude() < 1e-9);
}

TEST_CASE("ComputeCurve") {
    const Ellipsoid wgs84 = Ellipsoid::WGS84();
    const Vector3<double> p = wgs84.ToCartesian(Trig::DegreesToRadians(Geodetic2D(0.0, 0.0)));
    const Vector3<double> q = wgs84.ToCartesian(Trig::DegreesToRadians(Geodetic2D(40.0, 90.0)));
    const double granularity = Trig::DegreesToRadians(1.0);

    const auto curve = wgs84.ComputeCurve(p, q, granularity);

    SUBCASE("starts at p and ends at q") {
        REQUIRE(curve.size() >= 2);
        CHECK((curve.front() - p).Magnitude() == 0.0);
        CHECK((curve.back() - q).Magnitude() == 0.0);
    }

    SUBCASE("every point is on the surface") {
        for (const auto& point : curve) {
            CHECK(SurfaceEquation(wgs84, point) == doctest::Approx(1.0).epsilon(1e-12));
        }
    }

    SUBCASE("consecutive points are no more than granularity apart") {
        for (size_t i = 1; i < curve.size(); ++i) {
            CHECK(curve[i - 1].AngleBetween(curve[i]) <= granularity + 1e-12);
        }
    }

    SUBCASE("every point lies in the plane through the center, p and q") {
        const Vector3<double> normal = p.Cross(q).Normalize();
        for (const auto& point : curve) {
            CHECK(std::abs(point.Normalize().Dot(normal)) < 1e-12);
        }
    }

    SUBCASE("invalid input throws") {
        CHECK_THROWS_AS(wgs84.ComputeCurve(p, q, 0.0), std::invalid_argument);
        CHECK_THROWS_AS(wgs84.ComputeCurve(p, p * -1.0, granularity), std::invalid_argument);
        CHECK_THROWS_AS(wgs84.ComputeCurve(Vector3<double>::Zero(), q, granularity), std::invalid_argument);
    }

    SUBCASE("same direction returns just the endpoints") {
        CHECK(wgs84.ComputeCurve(p, p, granularity).size() == 2);
    }
}
