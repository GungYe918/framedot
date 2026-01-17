// include/framedot/gfx/SoftwareRenderer.hpp
/**
 * @file SoftwareRenderer.hpp
 * @brief RenderQueue(커맨드)를 PixelCanvas(실제 픽셀)로 변환하는 소프트웨어 렌더러.
 *
 * 향후:
 * - 타일링 + JobSystem으로 병렬 raster
 * - GPU backend(WebGPU/Vulkan)로 RenderQueue를 변환하는 경로 추가
 */
#pragma once
#include <framedot/gfx/RenderQueue.hpp>
#include <framedot/gfx/PixelCanvas.hpp>


namespace framedot::gfx {

    class SoftwareRenderer {
    public:
        /// @brief RenderQueue에 기록된 커맨드를 PixelCanvas로 래스터라이즈
        void execute(const RenderQueue& rq, PixelCanvas& out) noexcept;

    };

} // namespace framedot::gfx