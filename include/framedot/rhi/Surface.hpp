// include/framedot/rhi/Surface.hpp
#pragma once
#include <framedot/gfx/PixelCanvas.hpp>


namespace framedot::rhi {

    class Surface {
    public:
        virtual ~Surface() = default;
        virtual void present(const framedot::gfx::PixelCanvas& canvas) = 0;
    };

} // namespace framedot::rhi