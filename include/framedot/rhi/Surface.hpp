// include/framedot/rhi/Surface.hpp
/**
 * @file Surface.hpp
 * @brief 플랫폼 출력 어댑터 인터페이스.
 *
 * 엔진 내부의 픽셀 버퍼(PixelCanvas)를 직접 받지 않고,
 * 불변 뷰(PixelFrame)를 받아서 플랫폼별 변환/출력을 수행한다.
 */
#pragma once
#include <framedot/gfx/PixelFrame.hpp>


namespace framedot::rhi {

    class Surface {
    public:
        virtual ~Surface() = default;
        virtual void present(const framedot::gfx::PixelFrame& canvas) = 0;
    };

} // namespace framedot::rhi