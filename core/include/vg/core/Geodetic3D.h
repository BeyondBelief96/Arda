#pragma once

#include <cmath>
#include <vg/core/Geodetic2D.h>

namespace vg::core {
    class Geodetic3D {
        public:
            Geodetic3D(double latitude, double longitude, double altitude);
            Geodetic3D(Geodetic2D surfacePosition, double height);

            double Latitude() const { return latitude; }
            double Longitude() const { return longitude; }
            double Altitude() const { return altitude; }

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
