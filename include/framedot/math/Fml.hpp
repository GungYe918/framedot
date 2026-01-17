// include/framedot/math/Flm.hpp
#pragma once
#include <framedot/math/Types.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


namespace framedot::math::fml {

    using vec2 = ::glm::vec2;
    using vec3 = ::glm::vec3;
    using vec4 = ::glm::vec4;

    using mat3 = ::glm::mat3;
    using mat4 = ::glm::mat4;

    using quat = ::glm::quat;

    /// @brief framedot POD -> glm(임시) 변환
    inline vec2 to_value(Vec2f v) noexcept { return vec2(v.x, v.y); }
    inline vec3 to_value(Vec3f v) noexcept { return vec3(v.x, v.y, v.z); }
    inline vec4 to_value(Vec4f v) noexcept { return vec4(v.x, v.y, v.z, v.w); }

    /// @brief glm(임시) -> framedot POD 변환
    inline Vec2f from_value(vec2 v) noexcept { return {v.x, v.y}; }
    inline Vec3f from_value(vec3 v) noexcept { return {v.x, v.y, v.z}; }
    inline Vec4f from_value(vec4 v) noexcept { return {v.x, v.y, v.z, v.w}; }

    /// @brief 흔히 쓰는 변환
    /// - 복잡한 수식은 GLM, framedot은 호출 인터페이스만 유지
    inline mat4 make_trs_2d(Vec2f pos, float rot_rad, Vec2f scale) noexcept {
        mat4 m(1.0f);
        m = ::glm::translate(m, vec3(pos.x, pos.y, 0.0f));
        m = ::glm::rotate(m, rot_rad, vec3(0.0f, 0.0f, 1.0f));
        m = ::glm::scale(m, vec3(scale.x, scale.y, 1.0f));
        return m;
    }

} // namespace framedot::math::fml