#pragma once

#include <cmath>

namespace vg::core {
    class Geodetic2D {
        public:
            Geodetic2D() = default;
            Geodetic2D(double latitude, double longitude) : m_latitude(latitude), m_longitude(longitude) {}

            inline double Latitude() const { return m_latitude; }
            inline double Longitude() const { return m_longitude; }

            bool Equals(const Geodetic2D& other) const;
            bool EqualsEpsilon(const Geodetic2D& other, double epsilon);

            bool operator==(const Geodetic2D& other) const;
            bool operator !=(const Geodetic2D& other) const;


        private:
            double m_latitude = 0.0;
            double m_longitude = 0.0;
    };
}
