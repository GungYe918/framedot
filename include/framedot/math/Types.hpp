// framedot/math/Types.hpp
#pragma once
#include <cstdint>
#include <cmath>


namespace framedot::math {

    /// @brief 2D float 벡터
    struct Vec2f {
        float x{0};
        float y{0};

        constexpr Vec2f() = default;
        constexpr Vec2f(float _x, float _y) : x(_x), y(_y) {}
    };

    /// @brief 3D float 벡터
    struct Vec3f {
        float x{0};
        float y{0};
        float z{0};
    
        constexpr Vec3f() = default;
        constexpr Vec3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    };

    /// @brief 4D float 벡터
    struct Vec4f {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float w{0.0f};

        constexpr Vec4f() = default;
        constexpr Vec4f(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    };

    /// @brief 3x3 행렬
    /// - 2D 변환(회전/스케일/이동)을 3x3 동차좌표로 표현할 때 사용
    struct Mat3f {
        float m[9] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        };
    };

    /// @brief 4x4 행렬
    /// - 3D 변환 기본 형태
    struct Mat4f {
        float m[16] = {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
    };


    // ----------------------------
    // 기본 벡터 연산
    // ----------------------------

    /// @brief 벡터 덧셈
    constexpr Vec2f add(Vec2f a, Vec2f b) noexcept { return {a.x + b.x, a.y + b.y}; }
    constexpr Vec2f sub(Vec2f a, Vec2f b) noexcept { return {a.x - b.x, a.y - b.y}; }
    constexpr Vec2f mul(Vec2f a, float s) noexcept { return {a.x * s, a.y * s}; }

    constexpr Vec3f add(Vec3f a, Vec3f b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
    constexpr Vec3f sub(Vec3f a, Vec3f b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    constexpr Vec3f mul(Vec3f a, float s) noexcept { return {a.x * s, a.y * s, a.z * s}; }

    /// @brief 2D 내적
    constexpr float dot(Vec2f a, Vec2f b) noexcept { return a.x*b.x + a.y*b.y; }

    /// @brief 3D 내적
    constexpr float dot(Vec3f a, Vec3f b) noexcept { return a.x*b.x + a.y*b.y + a.z*b.z; }

    /// @brief 2D 길이
    inline float length(Vec2f v) noexcept { return std::sqrt(dot(v, v)); }

    /// @brief 3D 길이
    inline float length(Vec3f v) noexcept { return std::sqrt(dot(v, v)); }

    /// @brief 2D 정규화 (0 벡터면 0 반환)
    inline Vec2f normalize(Vec2f v) noexcept {
        const float len = length(v);
        if (len <= 0.0f) return {0.0f, 0.0f};
        return {v.x / len, v.y / len};
    }

    /// @brief 3D 정규화 (0 벡터면 0 반환)
    inline Vec3f normalize(Vec3f v) noexcept {
        const float len = length(v);
        if (len <= 0.0f) return {0.0f, 0.0f, 0.0f};
        return {v.x / len, v.y / len, v.z / len};
    }

    /// @brief 2D 외적의 z성분(스칼라) = a.x*b.y - a.y*b.x
    constexpr float cross_z(Vec2f a, Vec2f b) noexcept {
        return a.x*b.y - a.y*b.x;
    }

    /// @brief 3D 외적
    constexpr Vec3f cross(Vec3f a, Vec3f b) noexcept {
        return {
            a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x
        };
    }

} // namespace framedot::math