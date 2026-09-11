#pragma once

#include <cmath>

namespace vg::core {
    class Geodetic3D {
        public:
            Geodetic3D(double latitude, double longitude, double altitude)
                : latitude(latitude), longitude(longitude), altitude(altitude) {}

            double GetLatitude() const { return latitude; }
            double GetLongitude() const { return longitude; }
            double GetAltitude() const { return altitude; }

            bool Equals(const Geodetic3D &other) const;

            bool EqualsEpsilon(const Geodetic3D &other, double epsilon) const;

            bool operator==(const Geodetic3D &other) const;

            bool operator!=(const Geodetic3D &other) const;

        private:
            double latitude;
            double longitude;
            double altitude;
    };
}