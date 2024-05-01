#ifndef LOGICSIM_WIDGET_RENDER_WIDGET_H
#define LOGICSIM_WIDGET_RENDER_WIDGET_H

#include "vocabulary/render_mode.h"

#include <QImage>
#include <QWidget>

#include <string>

class BLImage;

namespace logicsim {

struct device_pixel_ratio_t;

namespace render_widget {

struct fallback_info_t {
    std::string message {};

    [[nodiscard]] explicit operator bool() const;
};

}  // namespace render_widget

/**
 * @brief: Widget for direct or buffered rendering via Blend2d.
 *
 * Direct rendering is especially useful for high DPI displays where
 * reaching high fps is challenging. Also display scaling can have an additional
 * performance impact.
 */
class RenderWidget : public QWidget {
    Q_OBJECT

   public:
    using fallback_info_t = render_widget::fallback_info_t;

    explicit RenderWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = {});

    auto set_requested_render_mode(RenderMode mode) -> void;
    [[nodiscard]] auto requested_render_mode() const -> RenderMode;

   protected:
    /**
     * @brief: Sets up BLImage with the render mode requested and calls renderEvent.
     */
    auto paintEvent(QPaintEvent*) -> void override;

   protected:
    /**
     * @brief: Called for each paintEvent to redraw the whole widget.
     *
     * @param bl_image           An image of the size of the widget. Content drawn to it
     *                           will be drawn on the widget itself.
     * @param device_pixel_ratio The device pixel ratio of the widget.
     * @param render_mode        The actual render mode used for this frame.
     * @param fallback_error     Message indicating the reason why the fallback is used.
     *
     * Note, this function needs to be implemented in derived classes.
     */
    virtual auto renderEvent(BLImage bl_image, device_pixel_ratio_t device_pixel_ratio,
                             RenderMode render_mode, fallback_info_t fallback_info)
        -> void = 0;

   private:
    QImage qt_image_ {};
    RenderMode requested_mode_ {RenderMode::direct};
};

}  // namespace logicsim

#endif
