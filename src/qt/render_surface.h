#ifndef LOGICSIM_QT_RENDER_SURFACE_H
#define LOGICSIM_QT_RENDER_SURFACE_H

#include "vocabulary/device_pixel_ratio.h"
#include "vocabulary/render_mode.h"

#include <QImage>

#include <functional>

class BLImage;
class QWidget;

namespace logicsim {

/**
 * @brief: Component used for direct or buffered rendering.
 */
class RenderSurface {
   public:
    /**
     * @brief: Free memory of the buffer.
     */
    auto reset() -> void;

    auto set_requested_mode(RenderMode mode) -> void;
    [[nodiscard]] auto requested_mode() const -> RenderMode;

    /**
     * @brief: Render function passed to paintEvent
     *
     * BLImage: Content drawn to this image will appear at the widget.
     * device_pixel_ratio_t: The device pixel ratio of the widget.
     * RenderMode: The actual render mode used for this frame.
     */
    using render_function_t =
        std::function<void(BLImage&, device_pixel_ratio_t, RenderMode)>;

    /**
     * @brief: Render the function with the mode requested, or its fallback..
     *
     * Note, buffered rendering has in generally little overhead,
     * except with display scaling enabled. This is bad as usually it is enabled
     * for high DPI displays where reaching high fps is challenging.
     *
     * Note paintEvent can only be called within a widgets paint-event.
     */
    auto paintEvent(QWidget& widget, render_function_t render_function) -> void;

   private:
    QImage qt_image_ {};
    RenderMode requested_mode_ {RenderMode::direct};
};

}  // namespace logicsim

#endif
