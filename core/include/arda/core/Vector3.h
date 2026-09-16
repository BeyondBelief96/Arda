#pragma once
#include <algorithm>
#include <concepts>
#include <iostream>
#include <type_traits>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace arda::core {
    template <typename T>
    class Vector3 {
        public:
            constexpr Vector3() = default;
            constexpr Vector3(T x, T y, T z) : m_x(x), m_y(y), m_z(z) {};

            static constexpr Vector3 Zero() { return Vector3(0, 0, 0); };
            static constexpr Vector3 UnitX() { return Vector3(1, 0, 0); };
            static constexpr Vector3 UnitY() { return Vector3(0, 1, 0); };
            static constexpr Vector3 UnitZ() { return Vector3(0, 0, 1); };
            static constexpr Vector3 Undefined() requires std::is_floating_point_v<T> {
                return {std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
            }

            constexpr T X() const { return m_x; };
            constexpr T Y() const { return m_y; };
            constexpr T Z() const { return m_z;};

            T Magnitude() const { return std::sqrt(m_x * m_x + m_y * m_y + m_z * m_z); }

            Vector3<T> Normalize() const {
                T magnitude = Magnitude();
                if(magnitude == 0) {
                    throw std::runtime_error("Cannot normalize a zero vector");
                }

                return Vector3(m_x / magnitude, m_y / magnitude, m_z / magnitude);
            }

            T Dot(const Vector3<T>& other) const {
                return m_x * other.X() + m_y * other.Y() + m_z * other.Z();
            }

            Vector3<T> Cross(const Vector3<T>& other) const {
                return Vector3(
                    m_y * other.Z() - m_z * other.Y(),
                    m_z * other.X() - m_x * other.Z(),
                    m_x * other.Y() - m_y * other.X()
                );
            }

            Vector3<T> MultiplyComponentWise(const Vector3<T>& other) const {
                return Vector3(m_x * other.X(), m_y * other.Y(), m_z * other.Z());
            }

            double AngleBetween(const Vector3<T>& other) const {
                // Clamp so rounding can't push the ratio outside acos's [-1, 1] domain.
                const double cosAngle = this->Dot(other) / (this->Magnitude() * other.Magnitude());
                return std::acos(std::clamp(cosAngle, -1.0, 1.0));
            }

            /**
             * Uses Rodrigues' rotation formula to rotate this vector by some angle about an arbitrary axis.
             * @param angle The amount of angle to rotate this vector by.
             * @param axis The axis about which to perform the rotation.
             * @returns The rotated vector.
             */
            Vector3<T> RotateAboutAxis(const double angle, const Vector3<T>& axis) const {
                const Vector3<T> k = axis.Normalize();

                const Vector3<T> rParallel = k * this->Dot(k); // Project this vector along the axis of rotation.
                const Vector3<T> rPerp = *this - rParallel;     // Component perpendicular to the axis.

                return rParallel + rPerp * std::cos(angle) + k.Cross(*this) * std::sin(angle);
            }

            /* Operator Overloads */

            Vector3<T> operator+(const Vector3<T>& other) const {
                return Vector3(m_x + other.X(), m_y + other.Y(), m_z + other.Z());
            }

            Vector3<T> operator-(const Vector3<T>& other) const {
                return Vector3(m_x - other.X(), m_y - other.Y(), m_z - other.Z());
            }

            Vector3<T> operator*(T scalar) const {
                return Vector3(m_x * scalar, m_y * scalar, m_z * scalar);
            }

            T operator*(const Vector3<T>& other) const {
                return this->Dot(other);
            }

            Vector3<T> operator/(T scalar) const {
                if(scalar == 0) {
                    throw std::runtime_error("Division by zero");
                }
                return Vector3(m_x / scalar, m_y / scalar, m_z / scalar);
            }
        private:
            T m_x;
            T m_y;
            T m_z;
    };

    template <typename T>
    Vector3<T> operator*(T scalar, const Vector3<T>& vector) {
        return vector * scalar;
    }
}
