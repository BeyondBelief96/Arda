#pragma once

namespace arda::core {
    template<typename T>
    class Vector4 {
        public:
            constexpr Vector4() = default;
            constexpr Vector4(T x, T y, T z, T w) : m_x(x), m_y(y), m_z(z), m_w(w) {};

            static constexpr Vector4 Zero() { return Vector4(0, 0, 0, 0); };
            static constexpr Vector4 UnitX() { return Vector4(1, 0, 0, 0); };
            static constexpr Vector4 UnitY() { return Vector4(0, 1, 0, 0); };
            static constexpr Vector4 UnitZ() { return Vector4(0, 0, 1, 0); };
            static constexpr Vector4 UnitW() { return Vector4(0, 0, 0, 1); };

            constexpr T X() const { return m_x; };
            constexpr T Y() const { return m_y; };
            constexpr T Z() const { return m_z;};
            constexpr T W() const { return m_w;};

            T Magnitude() const { return std::sqrt(m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w); }
            
            Vector4<T> Normalize() const {
                T magnitude = Magnitude();
                if(magnitude == 0) {
                    throw std::runtime_error("Cannot normalize a zero vector");
                }

                return Vector4(m_x / magnitude, m_y / magnitude, m_z / magnitude, m_w / magnitude);
            }

            Vector4<T> MultiplyComponentWise(const Vector4<T>& other) const {
                return Vector4(m_x * other.X(), m_y * other.Y(), m_z * other.Z(), m_w * other.W());
            }

            T Dot(const Vector4<T>& other) const {
                return m_x * other.X() + m_y * other.Y() + m_z * other.Z() + m_w * other.W();
            }

            /* Operator Overloads */
            Vector4<T> operator+(const Vector4<T>& other) const {
                return Vector4(m_x + other.X(), m_y + other.Y(), m_z + other.Z(), m_w + other.W());
            }

            Vector4<T> operator-(const Vector4<T>& other) const {
                return Vector4(m_x - other.X(), m_y - other.Y(), m_z - other.Z(), m_w - other.W());
            }

            Vector4<T> operator*(T scalar) const {
                return Vector4(m_x * scalar, m_y * scalar, m_z * scalar, m_w * scalar);
            }

            Vector4<T> operator/(T scalar) const {
                if(scalar == 0) {
                    throw std::runtime_error("Division by zero");
                }
                return Vector4(m_x / scalar, m_y / scalar, m_z / scalar, m_w / scalar);
            }

        private:
            T m_x{};
            T m_y{};
            T m_z{};
            T m_w{};
    };
}