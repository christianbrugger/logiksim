#include "widget/render_widget.h"

#include "format/blend2d_type.h"
#include "format/qt_type.h"
#include "qt/widget_geometry.h"
#include "vocabulary/device_pixel_ratio.h"
#include "vocabulary/fallback_info.h"

#include <blend2d.h>
#include <gsl/gsl>
#include <tl/expected.hpp>

#include <QBackingStore>
#include <QPainter>

namespace logicsim {

//
// Render Surface
//

namespace {

auto set_optimal_render_attributes(QWidget& widget) -> void {
    widget.setAutoFillBackground(false);
    widget.setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget.setAttribute(Qt::WA_NoSystemBackground, true);
}

}  // namespace

RenderWidget::RenderWidget(QWidget* parent, Qt::WindowFlags flags)
    : QWidget {parent, flags} {
    set_optimal_render_attributes(*this);
}

auto RenderWidget::set_requested_render_mode(RenderMode mode) -> void {
    requested_mode_ = mode;
}

auto RenderWidget::requested_render_mode() const -> RenderMode {
    return requested_mode_;
}

namespace {

auto bl_image_from_backing_store(QBackingStore* backing_store, GeometryInfo geometry_info)
    -> tl::expected<BLImage, std::string> {
    if (backing_store == nullptr) {
        return tl::unexpected("Given BackingStore is a nullptr.");
    }

    auto painting_device = backing_store->paintDevice();

    if (painting_device->paintingActive()) {
        return tl::unexpected("PaintingDevice has active painters unexpectedly.");
    }

    const auto image = dynamic_cast<QImage*>(painting_device);

    if (image == nullptr) {
        return tl::unexpected("Widget paintDevice is not a QImage.");
    }
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        const auto value = qToUnderlying(image->format());
        return tl::unexpected(
            fmt::format("Widget paintDevice has wrong QImage::Format of id {}.", value));
    }
    if (image->depth() != 32) {
        return tl::unexpected(fmt::format(
            "Widget paintDevice has an unexpected depth of {}.", image->depth()));
    }
    if (image->bitPlaneCount() != 32) {
        return tl::unexpected(
            fmt::format("Widget paintDevice has an unexpected bitPlaneCount of {}.",
                        image->bitPlaneCount()));
    }

    const auto rect = to_device_rounded(geometry_info);

    if (!image->rect().contains(rect)) {
        return tl::unexpected(
            fmt::format("Image with size {} is not able to contain device rect {}.",
                        image->rect(), rect));
    }

    // get pointer
    auto pixels_direct = image->constScanLine(rect.y());
    auto pixels = image->scanLine(rect.y());

    if (pixels == nullptr) {
        return tl::unexpected("Widget paintDevice data pointer is a nullptr.");
    }
    // QImage has copy-on-write behavior. That means constScanLine always gives us a
    // pointer to the buffer, while scanLine might make a copy first, if it is shared.
    // A modifying pointer to the original buffer is needed, which is check here.
    if (pixels != pixels_direct) {
        return tl::unexpected("Widget paintDevice data is shared.");
    }

    // shift by x
    static_assert(sizeof(*pixels) == 1);
    static_assert(CHAR_BIT == 8);
    pixels += std::ptrdiff_t {rect.x()} * std::ptrdiff_t {image->bitPlaneCount() / 8};

    auto result = tl::expected<BLImage, std::string> {BLImage {}};

    if (result.value().createFromData(rect.width(), rect.height(), BL_FORMAT_PRGB32,
                                      pixels, image->bytesPerLine()) != BL_SUCCESS) {
        return tl::unexpected("Unable to create BLImage, wrong parameters.");
    }

    return result;
}

auto resize_qt_image_no_copy(QImage& qt_image, QSize window_size) -> QImage {
    if (qt_image.size() != window_size) {
        qt_image = QImage {window_size.width(), window_size.height(),
                           QImage::Format_ARGB32_Premultiplied};
    }
    return qt_image;
}

auto bl_image_from_qt_image(QImage& qt_image) -> BLImage {
    auto bl_image = BLImage {};

    Expects(bl_image.createFromData(qt_image.width(), qt_image.height(), BL_FORMAT_PRGB32,
                                    qt_image.bits(),
                                    qt_image.bytesPerLine()) == BL_SUCCESS);

    return bl_image;
}

auto bl_image_from_qt_image(QImage& qt_image, GeometryInfo geometry_info) -> BLImage {
    resize_qt_image_no_copy(qt_image, to_size_device(geometry_info));

    return bl_image_from_qt_image(qt_image);
}

struct get_bl_image_result_t {
    BLImage bl_image {};
    RenderMode mode {RenderMode::buffered};
    fallback_info_t fallback_info {};
};

auto _get_bl_image(QBackingStore* backing_store, QImage& qt_image,
                   GeometryInfo geometry_info, RenderMode requested_mode)
    -> get_bl_image_result_t {
    // handle zero sizes
    if (const auto size = to_device_rounded(geometry_info);
        size.width() == 0 || size.height() == 0) {
        return get_bl_image_result_t {
            .bl_image = BLImage {},
            .mode = requested_mode,
            .fallback_info = {},
        };
    }

    switch (requested_mode) {
        case RenderMode::direct: {
            auto result_ = bl_image_from_backing_store(backing_store, geometry_info);

            if (result_.has_value()) {
                // free memory, as buffer is not needed
                qt_image = QImage {};

                return get_bl_image_result_t {
                    .bl_image = std::move(*result_),
                    .mode = RenderMode::direct,
                    .fallback_info = {},
                };
            }

            // buffered fallback
            return get_bl_image_result_t {
                .bl_image = bl_image_from_qt_image(qt_image, geometry_info),
                .mode = RenderMode::buffered,
                .fallback_info = {.message = std::move(result_.error())},
            };
        }

        case RenderMode::buffered: {
            return get_bl_image_result_t {
                .bl_image = bl_image_from_qt_image(qt_image, geometry_info),
                .mode = RenderMode::buffered,
                .fallback_info = {},
            };
        }
    }

    std::terminate();
}

auto expected_qt_image_size(RenderMode actual_mode, QSize size_device) -> QSize {
    switch (actual_mode) {
        using enum RenderMode;
        case direct:
            return QSize {0, 0};
        case buffered:
            return size_device;
    }

    std::terminate();
}

auto get_bl_image(QBackingStore* backing_store, QImage& qt_image,
                  GeometryInfo geometry_info, RenderMode requested_mode)
    -> get_bl_image_result_t {
    const auto result =
        _get_bl_image(backing_store, qt_image, geometry_info, requested_mode);

    const auto size_device_qt = to_size_device(geometry_info);
    const auto size_device_bl = BLSizeI {size_device_qt.width(), size_device_qt.height()};
    using enum RenderMode;
    Ensures(result.bl_image.size() == size_device_bl);
    Ensures(qt_image.size() == expected_qt_image_size(result.mode, size_device_qt));
    Ensures(!(requested_mode == buffered && result.mode != buffered));
    Ensures((requested_mode != result.mode) == bool {result.fallback_info});

    return result;
}

}  // namespace

auto RenderWidget::paintEvent(QPaintEvent* /*unused*/) -> void {
    const auto info = get_geometry_info(*this);
    auto result = get_bl_image(this->backingStore(), qt_image_, info, requested_mode_);

    renderEvent(std::move(result.bl_image),
                device_pixel_ratio_t {info.device_pixel_ratio}, result.mode,
                std::move(result.fallback_info));

    if (result.mode == RenderMode::buffered) {
        qt_image_.setDevicePixelRatio(info.device_pixel_ratio);
        auto painter = QPainter {this};
        painter.drawImage(QPoint(0, 0), qt_image_);
    }
}

}  // namespace logicsim
