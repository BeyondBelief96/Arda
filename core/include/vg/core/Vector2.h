#pragma once

#include <cmath>
#include <limits>
#include <concepts>

namespace vg::core {
    template <typename T> requires std::is_arithmetic_v<T>
    class Vector2 {
        public:
            constexpr Vector2() = default;
            constexpr Vector2(T x, T y) : m_x(x), m_y(y) {};

            static constexpr Vector2 Zero() { return Vector2(0, 0); };
            static constexpr Vector2 UnitX() { return Vector2(1, 0); };
            static constexpr Vector2 UnitY() { return Vector2(0, 1); };
            static constexpr Vector2 Undefined() requires std::is_floating_point_v<T> {
                return {std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
            }

            constexpr T X() const { return m_x; };
            constexpr T Y() const { return m_y; };
            
            T Magnitude() const { return std::sqrt(m_x * m_x + m_y * m_y); }
            
            T Dot(const Vector2<T>& other) const {
              return m_x * other.X() + m_y * other.Y();
            }

            Vector2<T> Normalize() const {
                T magnitude = Magnitude();
                if(magnitude == 0) {
                    throw std::runtime_error("Cannot normalize a zero vector");
                }

                return Vector2(m_x / magnitude, m_y / magnitude);
            }

            Vector2<T> MultiplyComponentWise(const Vector2<T>& other) const {
                return Vector2(m_x * other.X(), m_y * other.Y());
            }

            /* Operator Overloads */
            Vector2<T> operator+(const Vector2<T>& other) const {
                return Vector2(m_x + other.X(), m_y + other.Y());
            }

            Vector2<T> operator-(const Vector2<T>& other) const {
              return Vector2(m_x - other.X(), m_y - other.Y());
            }

            Vector2<T> operator*(T scalar) const {
                return Vector2(m_x * scalar, m_y * scalar);
            }

            Vector2<T> operator/(T scalar) const {
                if (scalar == 0) {
                    throw std::runtime_error("Division by zero");
                }
                return Vector2(m_x / scalar, m_y / scalar);
            }
        private:
            T m_x{};
            T m_y{};
    };
}