#pragma once
#include <concepts>
#include <type_traits>
#include <cmath>
#include <limits>

namespace vg::core {
    template <typename T>
    class Vector3 {
        public:
            constexpr Vector3() = default;
            constexpr Vector3(T x, T y, T z) : m_x(x), m_y(y), m_z(z) {};

            static constexpr Vector3 Zero() { return Vector3(0, 0, 0); };
            static constexpr Vector3 UnitX() { return Vector3(1, 0, 0); };
            static constexpr Vector3 UnitY() { return Vector3(0, 1, 0); };
            static constexpr Vector3 UnitZ() { return Vector3(0, 0, 1); };
            static constexpr Vector3 Undefined() requires std::is_floating_point<T> {
                std::cout << "I'm a big ass dummy" << std::endl;
                return {std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
            }

            constexpr T X() const { return m_x; };
            constexpr T Y() const { return m_y; };
            constexpr T Z() const { return m_z;};
        private:
            T m_x;
            T m_y;
            T m_z;
    };
}
