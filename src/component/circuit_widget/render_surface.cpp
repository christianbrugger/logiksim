#include "component/circuit_widget/render_surface.h"

#include "editable_circuit.h"
#include "logging.h"
#include "qt/widget_geometry.h"
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

auto RenderSurface::set_render_config(WidgetRenderConfig new_config) -> void {
    if (new_config == render_config_) {
        return;
    }

    context_.ctx.settings.thread_count = render_config_.thread_count;

    // update
    render_config_ = new_config;
}

auto RenderSurface::render_config() const -> const WidgetRenderConfig& {
    return render_config_;
}

auto RenderSurface::reset() -> void {
    context_.clear();
    context_.shrink_to_fit();
}

auto RenderSurface::view_config() const -> const ViewConfig& {
    return context_.ctx.settings.view_config;
}

auto RenderSurface::set_view_point(ViewPoint view_point) -> void {
    context_.ctx.settings.view_config.set_view_point(view_point);
}

auto RenderSurface::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    context_.ctx.settings.view_config.set_device_pixel_ratio(device_pixel_ratio);
}

auto RenderSurface::statistics() const -> SurfaceStatistics {
    return SurfaceStatistics {
        .frames_per_second = fps_counter_.events_per_second(),
        .pixel_scale = context_.ctx.settings.view_config.pixel_scale(),
        .image_size = last_render_size_,
        .uses_direct_rendering = qt_image_.width() == 0 && qt_image_.height() == 0,
    };
}

namespace {

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

    const auto rect = to_device_rounded(geometry_info, image->rect());

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

auto get_bl_image(QBackingStore* backing_store, QImage& qt_image,
                  GeometryInfo geometry_info, bool direct_rendering) -> BLImage {
    if (direct_rendering) {
        if (auto result = bl_image_from_backing_store(backing_store, geometry_info)) {
            qt_image = QImage {};
            return std::move(*result);
        } else {
            print("WARNING: Cannot use direct rendering:", result.error());
        }
    }

    resize_qt_image(qt_image, to_size_device(geometry_info));
    return bl_image_from_qt_image(qt_image);
}

}  // namespace

auto RenderSurface::begin_paint(QBackingStore* backing_store, GeometryInfo geometry_info)
    -> CircuitContext& {
    set_device_pixel_ratio(geometry_info.device_pixel_ratio);

    context_.ctx.bl_image = get_bl_image(backing_store, qt_image_, geometry_info,
                                         render_config_.direct_rendering);
    context_.ctx.begin();
    last_render_size_ = context_.ctx.bl_image.size();

    return context_;
}

auto RenderSurface::end_paint(QPaintDevice& paint_device) -> void {
    context_.ctx.end();
    context_.ctx.bl_image = BLImage {};

    // we need to use QPainter if we are not directly drawing to the backend store
    // this mostly acceptable, but slow with display scaling
    if (qt_image_.width() != 0) {
        qt_image_.setDevicePixelRatio(view_config().device_pixel_ratio());
        auto painter = QPainter {&paint_device};
        painter.drawImage(QPoint(0, 0), qt_image_);
    }

    fps_counter_.count_event();
}

//
// Free Functions
//

auto set_view_config_offset(RenderSurface& render_surface, point_fine_t offset) -> void {
    auto view_point = render_surface.view_config().view_point();
    view_point.offset = offset;
    render_surface.set_view_point(view_point);
}

auto set_view_config_device_scale(RenderSurface& render_surface, double device_scale)
    -> void {
    auto view_point = render_surface.view_config().view_point();
    view_point.device_scale = device_scale;
    render_surface.set_view_point(view_point);
}

auto set_optimal_render_attributes(QWidget& widget) -> void {
    widget.setAutoFillBackground(false);
    widget.setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget.setAttribute(Qt::WA_NoSystemBackground, true);
}

namespace {

auto render_circuit_background(CircuitContext& context) {
    // print_fmt("Layers: {:.3f} MB\n", context_.layers.allocated_size() / 1024. / 1024.);

    render_background(context.ctx);
}

auto render_circuit_overlay(CircuitContext& context) {
    // context_.ctx.bl_ctx.setFillStyle(BLRgba32(defaults::color_black.value));
    // context_.ctx.bl_ctx.fillRect(BLRect {0, 0, 1, 100});
    // context_.ctx.bl_ctx.fillRect(BLRect {context_.ctx.bl_image.width() - 1.0, 0, 1,
    // 100}); context_.ctx.bl_ctx.fillRect(BLRect {0, 0, 100, 1});
    // context_.ctx.bl_ctx.fillRect(
    //     BLRect {0, context_.ctx.bl_image.height() - 1.0, 100, 1});
}

}  // namespace

auto render_to_context(CircuitContext& context, const WidgetRenderConfig &render_config,
                       const Layout &layout) -> void {
    render_circuit_background(context);

    if (render_config.show_circuit) {
        render_layout(context, layout);
    }

    render_circuit_overlay(context);
}

auto render_to_context(CircuitContext& context, const WidgetRenderConfig &render_config,
                       const EditableCircuit &editable_circuit, bool show_size_handles)
    -> void {
    render_circuit_background(context);

    // print_fmt("Layout: {:.3f} MB\n",
    //           editable_circuit_.value().layout().allocated_size() / 1024. / 1024.);
    // print_fmt("Circuit Caches: {:.3f} MB\n",
    //           editable_circuit_.value().caches().allocated_size() / 1024. / 1024.);

    if (render_config.show_circuit) {
        // TODO don't pull out references
        const auto& target_layout = editable_circuit.layout();
        const auto& selection = editable_circuit.visible_selection();

        render_layout(context, target_layout, selection);

        render_setting_handle(context.ctx, target_layout, selection);

        if (show_size_handles) {
            print("not implemented");
        }

        // if (show_size_handles) {
        //     render_size_handles(context_.ctx, target_layout, selection);
        // }

        // if (!(mouse_logic_ &&
        //       std::holds_alternative<MouseAreaSelectionLogic>(mouse_logic_.value())))
        //       {
        //     render_size_handles(context_.ctx, layout, selection);
        // }
    }

    
    if (render_config.show_collision_cache) {
        render_editable_circuit_collision_cache(context.ctx, editable_circuit);
    }
    if (render_config.show_connection_cache) {
        render_editable_circuit_connection_cache(context.ctx, editable_circuit);
    }
    if (render_config.show_selection_cache) {
        render_editable_circuit_selection_cache(context.ctx, editable_circuit);
    }

    render_circuit_overlay(context);
}

auto render_to_context(CircuitContext& context, const WidgetRenderConfig &render_config,
                       const SpatialSimulation &spatial_simulation) -> void {
    render_circuit_background(context);

    if (render_config.show_circuit) {
        // TODO Simulation view should contain layout, remove double reference
        render_simulation(context, spatial_simulation.layout(),
                          SimulationView {spatial_simulation});
    }

    render_circuit_overlay(context);
}

}  // namespace circuit_widget

}  // namespace logicsim
