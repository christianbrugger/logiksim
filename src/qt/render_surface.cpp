#include "qt/render_surface.h"

#include "logging.h"
#include "qt/widget_geometry.h"

#include <blend2d.h>
#include <gsl/gsl>
#include <tl/expected.hpp>

#include <QBackingStore>
#include <QPainter>
#include <QWidget>

namespace logicsim {

fallback_error_t::operator bool() const {
    return !message.empty();
}

//
// Render Surface
//

auto RenderSurface::reset() -> void {
    qt_image_ = QImage {};
}

auto RenderSurface::set_requested_mode(RenderMode mode) -> void {
    requested_mode_ = mode;
}

auto RenderSurface::requested_mode() const -> RenderMode {
    return requested_mode_;
}

namespace {

auto set_optimal_render_attributes(QWidget& widget) -> void {
    widget.setAutoFillBackground(false);
    widget.setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget.setAttribute(Qt::WA_NoSystemBackground, true);
}

auto bl_image_from_backing_store(QBackingStore* backing_store, GeometryInfo geometry_info)
    -> tl::expected<BLImage, std::string> {
    if (backing_store == nullptr) {
        return tl::unexpected("BackingStore is null.");
    }

    auto painting_device = backing_store->paintDevice();

    if (painting_device->paintingActive()) {
        return tl::unexpected("PaintingDevice is already used.");
    }

    const auto image = dynamic_cast<QImage*>(painting_device);

    if (image == nullptr) {
        return tl::unexpected("Widget paintDevice is not a QImage.");
    }
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        return tl::unexpected("Widget paintDevice has the wrong format.");
    }
    if (image->depth() != 32) {
        return tl::unexpected("Widget paintDevice has an unexpected depth.");
    }
    if (image->bitPlaneCount() != 32) {
        return tl::unexpected("Widget paintDevice has an unexpected bitPlaneCount.");
    }

    const auto rect = to_device_rounded(geometry_info, image->rect());
    Expects(image->rect().contains(rect));

    // get pointer
    auto pixels_direct = image->constScanLine(rect.y());
    auto pixels = image->scanLine(rect.y());

    if (pixels == nullptr) {
        return tl::unexpected("Widget paintDevice data pointer is null.");
    }
    // scanLine can make a deep copy, we don't want that, constScanLine never does
    // we query that one so if pointers are the same, we know there was no copy made.
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
        return tl::unexpected("Unable to create BLImage, wrong parameters");
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

    bl_image.createFromData(qt_image.width(), qt_image.height(), BL_FORMAT_PRGB32,
                            qt_image.bits(), qt_image.bytesPerLine());

    return bl_image;
}

auto bl_image_from_qt_image(QImage& qt_image, GeometryInfo geometry_info) -> BLImage {
    resize_qt_image_no_copy(qt_image, to_size_device(geometry_info));

    return bl_image_from_qt_image(qt_image);
}

struct get_bl_image_result_t {
    BLImage image {};
    RenderMode mode {RenderMode::buffered};
    fallback_error_t fallback_error {};
};

auto _get_bl_image(QBackingStore* backing_store, QImage& qt_image,
                   GeometryInfo geometry_info, RenderMode requested_mode)
    -> get_bl_image_result_t {
    if (requested_mode == RenderMode::direct) {
        auto result_ = bl_image_from_backing_store(backing_store, geometry_info);

        if (result_) {
            // free memory, as buffer is not needed
            qt_image = QImage {};

            return get_bl_image_result_t {
                .image = std::move(*result_),
                .mode = RenderMode::direct,
                .fallback_error = {},
            };
        }

        return get_bl_image_result_t {
            .image = bl_image_from_qt_image(qt_image, geometry_info),
            .mode = RenderMode::buffered,
            .fallback_error = fallback_error_t {.message = result_.error()},
        };
    }

    return get_bl_image_result_t {
        .image = bl_image_from_qt_image(qt_image, geometry_info),
        .mode = RenderMode::buffered,
        .fallback_error = {},
    };
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

    Ensures(result.image.size() == size_device_bl);
    Ensures(qt_image.size() == expected_qt_image_size(result.mode, size_device_qt));
    Ensures(
        !(requested_mode == RenderMode::buffered && result.mode == RenderMode::direct));
    Ensures((requested_mode == result.mode) == result.fallback_error.message.empty());

    return result;
}

}  // namespace

auto RenderSurface::paintEvent(QWidget& widget, render_function_t render_function)
    -> void {
    set_optimal_render_attributes(widget);

    const auto info = get_geometry_info(widget);
    auto result = get_bl_image(widget.backingStore(), qt_image_, info, requested_mode_);

    // TODO !!! use device_pixel_ratio_t in more places
    render_function(result.image, device_pixel_ratio_t {info.device_pixel_ratio},
                    result.mode, std::move(result.fallback_error));

    if (result.mode == RenderMode::buffered) {
        qt_image_.setDevicePixelRatio(info.device_pixel_ratio);
        auto painter = QPainter {&widget};
        painter.drawImage(QPoint(0, 0), qt_image_);
    }
}

}  // namespace logicsim
