#include "component/circuit_widget/circuit_renderer.h"

#include "editable_circuit.h"
#include "format/blend2d_type.h"  // TODO remove
#include "logging.h"
#include "qt/widget_geometry.h"
#include "render_caches.h"
#include "render_circuit.h"
#include "simulation_view.h"
#include "spatial_simulation.h"

#include <tl/expected.hpp>

namespace logicsim {

namespace circuit_widget {

auto SurfaceStatistics::format() const -> std::string {
    return fmt::format(
        "SurfaceStatistics{{\n"
        "  frames_per_second = {},\n"
        "  pixel_scale = {},\n"
        "  image_size = {}x{}px\n"
        "}}",
        frames_per_second, pixel_scale, image_size.w, image_size.h);
}

auto CircuitRenderer::set_render_config(WidgetRenderConfig new_config) -> void {
    if (new_config == render_config_) {
        return;
    }

    context_settings_.thread_count = new_config.thread_count;

    // update
    render_config_ = new_config;
}

auto CircuitRenderer::render_config() const -> const WidgetRenderConfig& {
    return render_config_;
}

auto CircuitRenderer::reset() -> void {
    context_surface_ = ImageSurface {};
    context_cache_.clear();

    fps_counter_ = EventCounter {};
    last_render_size_ = BLSize {};
}

auto CircuitRenderer::view_config() const -> const ViewConfig& {
    return context_settings_.view_config;
}

auto CircuitRenderer::set_view_point(const ViewPoint& view_point) -> void {
    context_settings_.view_config.set_view_point(view_point);
}

auto CircuitRenderer::set_device_pixel_ratio(double device_pixel_ratio) -> void {
    context_settings_.view_config.set_device_pixel_ratio(device_pixel_ratio);
}

auto CircuitRenderer::statistics() const -> SurfaceStatistics {
    return SurfaceStatistics {
        .frames_per_second = fps_counter_.events_per_second(),
        .pixel_scale = context_settings_.view_config.pixel_scale(),
        .image_size = last_render_size_,
    };
}

auto CircuitRenderer::count_frame(BLSizeI image_size) -> void {
    fps_counter_.count_event();
    last_render_size_ = image_size;
}

auto CircuitRenderer::render_layout(BLImage& bl_image, const Layout& layout) -> void {
    // TODO not the right place, generate ContextRenderSettings
    context_settings_.view_config.set_size(bl_image.size());

    render_to_image(bl_image, context_settings_, context_cache_, [&](Context& ctx) {
        render_to_context(ctx, context_surface_, render_config_, layout);
    });

    count_frame(bl_image.size());
}

auto CircuitRenderer::render_editable_circuit(BLImage& bl_image,
                                              const EditableCircuit& editable_circuit,
                                              bool show_size_handles) -> void {
    // TODO not the right place, generate ContextRenderSettings
    context_settings_.view_config.set_size(bl_image.size());

    render_to_image(bl_image, context_settings_, context_cache_, [&](Context& ctx) {
        render_to_context(ctx, context_surface_, render_config_, editable_circuit,
                          show_size_handles);
    });

    count_frame(bl_image.size());
}

auto CircuitRenderer::render_simulation(BLImage& bl_image,
                                        const SpatialSimulation& spatial_simulation)
    -> void {
    // TODO not the right place, generate ContextRenderSettings
    context_settings_.view_config.set_size(bl_image.size());

    render_to_image(bl_image, context_settings_, context_cache_, [&](Context& ctx) {
        render_to_context(ctx, render_config_, spatial_simulation);
    });

    count_frame(bl_image.size());
}

//
// Free Functions
//

auto set_view_config_offset(CircuitRenderer& render_surface, point_fine_t offset)
    -> void {
    auto view_point = render_surface.view_config().view_point();
    view_point.offset = offset;
    render_surface.set_view_point(view_point);
}

auto set_view_config_device_scale(CircuitRenderer& render_surface, double device_scale)
    -> void {
    auto view_point = render_surface.view_config().view_point();
    view_point.device_scale = device_scale;
    render_surface.set_view_point(view_point);
}

namespace {

auto render_circuit_background(Context& ctx) {
    // print_fmt("Layers: {:.3f} MB\n", context_.layers.allocated_size() / 1024. / 1024.);

    render_background(ctx);
}

auto render_circuit_overlay(Context& ctx [[maybe_unused]]) {
    // context_.ctx.bl_ctx.setFillStyle(BLRgba32(defaults::color_black.value));
    // context_.ctx.bl_ctx.fillRect(BLRect {0, 0, 1, 100});
    // context_.ctx.bl_ctx.fillRect(BLRect {context_.ctx.bl_image.width() - 1.0, 0, 1,
    // 100}); context_.ctx.bl_ctx.fillRect(BLRect {0, 0, 100, 1});
    // context_.ctx.bl_ctx.fillRect(
    //     BLRect {0, context_.ctx.bl_image.height() - 1.0, 100, 1});
}

}  // namespace

auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config, const Layout& layout)
    -> void {
    render_circuit_background(ctx);

    if (render_config.show_circuit) {
        // TODO write version that does not need surface ?
        render_layout(ctx, surface, layout);
    }

    render_circuit_overlay(ctx);
}

auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config,
                       const EditableCircuit& editable_circuit, bool show_size_handles)
    -> void {
    render_circuit_background(ctx);

    // print_fmt("Layout: {:.3f} MB\n",
    //           editable_circuit_.value().layout().allocated_size() / 1024. / 1024.);
    // print_fmt("Circuit Caches: {:.3f} MB\n",
    //           editable_circuit_.value().caches().allocated_size() / 1024. / 1024.);

    if (render_config.show_circuit) {
        // TODO don't pull out references
        const auto& target_layout = editable_circuit.layout();
        const auto& selection = editable_circuit.visible_selection();

        render_layout(ctx, surface, target_layout, selection);

        render_setting_handle(ctx, target_layout, selection);

        if (show_size_handles) {
            render_size_handles(ctx, target_layout, selection);
        }

        // if (!(mouse_logic_ &&
        //       std::holds_alternative<MouseAreaSelectionLogic>(mouse_logic_.value())))
        //       {
        //     render_size_handles(context_.ctx, layout, selection);
        // }
    }

    if (render_config.show_collision_cache) {
        render_editable_circuit_collision_cache(ctx, editable_circuit);
    }
    if (render_config.show_connection_cache) {
        render_editable_circuit_connection_cache(ctx, editable_circuit);
    }
    if (render_config.show_selection_cache) {
        render_editable_circuit_selection_cache(ctx, editable_circuit);
    }

    render_circuit_overlay(ctx);
}

auto render_to_context(Context& ctx, const WidgetRenderConfig& render_config,
                       const SpatialSimulation& spatial_simulation) -> void {
    render_circuit_background(ctx);

    if (render_config.show_circuit) {
        // TODO Simulation view should contain layout, remove double reference
        render_simulation(ctx, spatial_simulation.layout(),
                          SimulationView {spatial_simulation});
    }

    render_circuit_overlay(ctx);
}

}  // namespace circuit_widget

}  // namespace logicsim
