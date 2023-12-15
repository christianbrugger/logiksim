#include "component/circuit_widget/render_surface.h"

#include "editable_circuit.h"
#include "logging.h"
#include "render_caches.h"
#include "render_circuit.h"
#include "simulation_view.h"
#include "spatial_simulation.h"

#include <tl/expected.hpp>

#include <QBackingStore>
#include <QPainter>
#include <QWidget>

namespace logicsim {

namespace circuit_widget {

auto SurfaceStatistics::format() const -> std::string {
    return fmt::format(
        "SurfaceStatistics{{\n"
        "  frames_per_second = {},\n"
        "  pixel_scale = {},\n"
        "  image_size = {}x{}px\n"
        "  uses_direct_rendering = {}\n"
        "}}",
        frames_per_second, pixel_scale, image_size.w, image_size.h,
        uses_direct_rendering);
}

namespace {

auto round_logical_to_device(QPointF p, double pixel_ratio,
                             std::optional<QRect> clip = {}) -> QPoint {
    auto dx = gsl::narrow<int>(std::floor(p.x() * pixel_ratio + 0.5));
    auto dy = gsl::narrow<int>(std::floor(p.y() * pixel_ratio + 0.5));

    if (clip && false) {
        dx = std::clamp(dx, clip->x(), clip->x() + clip->width());
        dy = std::clamp(dy, clip->y(), clip->y() + clip->height());
    }

    return QPoint {dx, dy};
}

auto round_logical_to_device(QRectF rect, double pixel_ratio,
                             std::optional<QRect> clip = {}) -> QRect {
    const auto p0_logic = QPoint(rect.x(), rect.y());
    const auto p1_logic = QPoint(rect.x() + rect.width(), rect.y() + rect.height());

    const auto p0 = round_logical_to_device(p0_logic, pixel_ratio, clip);
    const auto p1 = round_logical_to_device(p1_logic, pixel_ratio, clip);

    return QRect(p0.x(), p0.y(), p1.x() - p0.x(), p1.y() - p0.y());
}

auto geometry_top_level(const QWidget& widget) -> QRect {
    const auto geometry = widget.geometry();
    const auto top_left = widget.mapTo(widget.topLevelWidget(), QPoint {0, 0});

    return QRect {top_left.x(), top_left.y(), geometry.width(), geometry.height()};
}

auto size_device(const QWidget& widget) -> QSize {
    const auto geometry = geometry_top_level(widget);
    const auto pixel_ratio = widget.devicePixelRatioF();

    return round_logical_to_device(geometry, pixel_ratio).size();
}

}  // namespace

auto RenderSurface::set_render_config(WidgetRenderConfig new_config) -> void {
    if (new_config == render_config_) {
        return;
    }

    context_.ctx.settings.thread_count = render_config_.thread_count;

    // update
    render_config_ = new_config;
}

auto RenderSurface::reset() -> void {
    context_.clear();
    context_.shrink_to_fit();
}

auto RenderSurface::view_config() const -> const ViewConfig& {
    return context_.ctx.settings.view_config;
}

auto RenderSurface::set_view_config_offset(point_fine_t offset) -> void {
    context_.ctx.settings.view_config.set_offset(offset);
}

auto RenderSurface::set_view_config_device_scale(double scale) -> void {
    context_.ctx.settings.view_config.set_device_scale(scale);
}

auto RenderSurface::set_view_point(ViewPoint view_point) -> void {
    context_.ctx.settings.view_config.set_view_point(view_point);
}

auto RenderSurface::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    context_.ctx.settings.view_config.set_device_pixel_ratio(device_pixel_ratio);
    qt_image_.setDevicePixelRatio(device_pixel_ratio);
}

auto RenderSurface::statistics() const -> SurfaceStatistics {
    return SurfaceStatistics {
        .frames_per_second = fps_counter_.events_per_second(),
        .pixel_scale = context_.ctx.settings.view_config.pixel_scale(),
        .image_size = context_.ctx.bl_ctx.targetSize(),
        .uses_direct_rendering = qt_image_.width() == 0 && qt_image_.height() == 0 &&
                                 context_.ctx.bl_image.width() != 0 &&
                                 context_.ctx.bl_image.height() != 0,
    };
}

namespace {

auto bl_image_from_backing_store(QWidget& widget) -> tl::expected<BLImage, std::string> {
    const auto backing_store = widget.backingStore();

    if (backing_store == nullptr) {
        return tl::unexpected("Widget backing_store pointer is null.");
    }

    const auto image = dynamic_cast<QImage*>(backing_store->paintDevice());

    if (image == nullptr) {
        return tl::unexpected("Widget paintDevice is not a QImage.");
    }
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        return tl::unexpected("Widget paintDevice has the wrong format.");
    }
    if (image->depth() != 32) {
        return tl::unexpected("Widget paintDevice has an unexpected depth.");
    }

    const auto rect = round_logical_to_device(geometry_top_level(widget),
                                              image->devicePixelRatioF(), image->rect());

    // print(geometry().x() * image->devicePixelRatioF(),                         //
    //       geometry().y() * image->devicePixelRatioF(),                         //
    //       (geometry().x() + geometry().width()) * image->devicePixelRatioF(),  //
    //       (geometry().y() + geometry().height()) * image->devicePixelRatioF()  //
    //);

    // get pointer
    auto pixels_direct = image->constScanLine(rect.y());
    auto pixels = image->scanLine(rect.y());

    if (pixels == nullptr) {
        return tl::unexpected("Widget paintDevice data pointer is null.");
    }
    // scanLine can make a deep copy, we don't want that, constScanLine never does
    if (pixels != pixels_direct) {
        return tl::unexpected("Widget paintDevice data is shared.");
    }

    // shift by x
    static_assert(sizeof(*pixels) == 1);
    pixels += rect.x() * (image->depth() / 8);

    auto result = tl::expected<BLImage, std::string> {BLImage {}};
    if (result.value().createFromData(rect.width(), rect.height(), BL_FORMAT_PRGB32,
                                      pixels, image->bytesPerLine()) != BL_SUCCESS) {
        return tl::unexpected("Unable to create BLImage, wrong parameters");
    }
    return result;
}

auto resize_qt_image(QImage& qt_image, QSize window_size) -> QImage {
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

auto get_bl_image(bool direct_rendering, QWidget& widget, QImage& qt_image) -> BLImage {
    if (direct_rendering) {
        if (auto result = bl_image_from_backing_store(widget)) {
            return std::move(*result);
        } else {
            print("WARNING: Cannot use direct rendering:", result.error());
        }
    }
    return bl_image_from_qt_image(qt_image);
}

auto render_to_context(CircuitContext& context, const WidgetRenderConfig render_config,
                       const EditableCircuit* editable_circuit,
                       const SpatialSimulation* spatial_simulation, const Layout* layout,
                       bool show_size_handles) {
    // print_fmt("Layers: {:.3f} MB\n", context_.layers.allocated_size() / 1024. / 1024.);
    // print_fmt("Layout: {:.3f} MB\n",
    //           editable_circuit_.value().layout().allocated_size() / 1024. / 1024.);
    // print_fmt("Caches: {:.3f} MB\n",
    //           editable_circuit_.value().caches().allocated_size() / 1024. / 1024.);

    render_background(context.ctx);

    if (render_config.show_circuit) {
        if (editable_circuit) {
            const auto& target_layout = editable_circuit->layout();
            const auto& selection = editable_circuit->visible_selection();

            render_layout(context, target_layout, selection);

            render_setting_handle(context.ctx, target_layout, selection);

            // if (show_size_handles) {
            //     render_size_handles(context_.ctx, target_layout, selection);
            // }

            // if (!(mouse_logic_ &&
            //       std::holds_alternative<MouseAreaSelectionLogic>(mouse_logic_.value())))
            //       {
            //     render_size_handles(context_.ctx, layout, selection);
            // }
        }

        else if (spatial_simulation) {
            render_simulation(context, spatial_simulation->layout(),
                              SimulationView {*spatial_simulation});
        }

        else if (layout) {
            render_layout(context, *layout);
        }
    }

    if (render_config.show_collision_cache && editable_circuit) {
        render_editable_circuit_collision_cache(context.ctx, *editable_circuit);
    }
    if (render_config.show_connection_cache && editable_circuit) {
        render_editable_circuit_connection_cache(context.ctx, *editable_circuit);
    }
    if (render_config.show_selection_cache && editable_circuit) {
        render_editable_circuit_selection_cache(context.ctx, *editable_circuit);
    }

    // context_.ctx.bl_ctx.setFillStyle(BLRgba32(defaults::color_black.value));
    // context_.ctx.bl_ctx.fillRect(BLRect {0, 0, 1, 100});
    // context_.ctx.bl_ctx.fillRect(BLRect {context_.ctx.bl_image.width() - 1.0, 0, 1,
    // 100}); context_.ctx.bl_ctx.fillRect(BLRect {0, 0, 100, 1});
    // context_.ctx.bl_ctx.fillRect(
    //     BLRect {0, context_.ctx.bl_image.height() - 1.0, 100, 1});
}

}  // namespace

auto RenderSurface::paintEvent(QWidget& widget, const EditableCircuit* editable_circuit,
                               const SpatialSimulation* spatial_simulation,
                               const Layout* layout, bool show_size_handles) -> void {
    if (!widget.isVisible()) {
        return;
    }

    set_optimal_render_attributes(widget);
    resize_qt_image(qt_image_, size_device(widget));

    auto bl_image = get_bl_image(render_config_.direct_rendering, widget, qt_image_);
    context_.ctx.begin(bl_image);

    render_to_context(context_, render_config_, editable_circuit, spatial_simulation,
                      layout, show_size_handles);

    context_.ctx.end();

    // we use QPainter only if we are directly drawing
    if (qt_image_.width() != 0) {
        auto painter = QPainter {&widget};
        painter.drawImage(QPoint(0, 0), qt_image_);
    }

    fps_counter_.count_event();
}

auto set_optimal_render_attributes(QWidget& widget) -> void {
    widget.setAutoFillBackground(false);
    widget.setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget.setAttribute(Qt::WA_NoSystemBackground, true);
}

}  // namespace circuit_widget

}  // namespace logicsim
