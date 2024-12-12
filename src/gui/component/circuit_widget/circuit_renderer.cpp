#include "gui/component/circuit_widget/circuit_renderer.h"

#include "core/editable_circuit.h"
#include "core/geometry/scene.h"
#include "core/render/circuit/render_background.h"
#include "core/render/circuit/render_circuit.h"
#include "core/render/circuit/render_layout_index.h"
#include "core/render/circuit/render_setting_handle.h"
#include "core/render/circuit/render_size_handle.h"
#include "core/render/text_cache.h"
#include "core/spatial_simulation.h"
#include "core/vocabulary/allocation_info.h"

#include <gsl/gsl>

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

CircuitRenderer::CircuitRenderer() {
    set_render_config(render_config_);
}

auto CircuitRenderer::allocation_info() const -> CircuitRendererAllocInfo {
    return CircuitRendererAllocInfo {
        .image_surface = Byte {context_surface_.allocated_size()},
        .context_cache = context_cache_.allocation_info(),
    };
}

auto CircuitRenderer::set_render_config(WidgetRenderConfig new_config) -> void {
    context_settings_.thread_count = new_config.thread_count;
    context_settings_.jit_rendering = new_config.jit_rendering;
    context_settings_.wire_render_style = new_config.wire_render_style;

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

auto CircuitRenderer::set_mouse_position_info(std::optional<MousePositionInfo> info)
    -> void {
    mouse_position_info_ = std::move(info);
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
        render_mouse_position_info(ctx, render_config_, mouse_position_info_);
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
        render_mouse_position_info(ctx, render_config_, mouse_position_info_);
    });

    count_frame(bl_image.size());
}

auto CircuitRenderer::render_simulation(
    BLImage& bl_image, const SpatialSimulation& spatial_simulation) -> void {
    // TODO not the right place, generate ContextRenderSettings
    context_settings_.view_config.set_size(bl_image.size());

    render_to_image(bl_image, context_settings_, context_cache_, [&](Context& ctx) {
        render_to_context(ctx, render_config_, spatial_simulation);
        render_mouse_position_info(ctx, render_config_, mouse_position_info_);
    });

    count_frame(bl_image.size());
}

//
// Free Functions
//

auto set_view_config_offset(CircuitRenderer& render_surface,
                            point_fine_t offset) -> void {
    auto view_point = render_surface.view_config().view_point();
    view_point.offset = offset;
    render_surface.set_view_point(view_point);
}

auto set_view_config_device_scale(CircuitRenderer& render_surface,
                                  double device_scale) -> void {
    auto view_point = render_surface.view_config().view_point();
    view_point.device_scale = device_scale;
    render_surface.set_view_point(view_point);
}

namespace {

auto render_circuit_background(Context& ctx) {
    render_background(ctx);
}

namespace {

/**
 * @brief: Draw narrow lines around the target borders to ensure all is visible.
 */
auto draw_target_outline(Context& ctx, int margin, color_t color) {
    if (margin < 0) [[unlikely]] {
        throw std::runtime_error("margin needs to be positive");
    }

    const auto size = ctx.bl_ctx.targetSize();

    // length of marker
    const auto d = double {100};

    // first x and y
    const auto x1 = gsl::narrow<double>(0 + margin);
    const auto y1 = gsl::narrow<double>(0 + margin);
    // last x and y
    const auto x2 = gsl::narrow<double>(size.w - 1 - margin);
    const auto y2 = gsl::narrow<double>(size.h - 1 - margin);

    // upper left
    ctx.bl_ctx.fillRect(BLRect {x1, y1, 0 + 1, d + 1}, color);
    ctx.bl_ctx.fillRect(BLRect {x1, y1, d + 1, 0 + 1}, color);
    // lower left
    ctx.bl_ctx.fillRect(BLRect {x1, y2 - d, 0 + 1, d + 1}, color);
    ctx.bl_ctx.fillRect(BLRect {x1, y2 + 0, d + 1, 0 + 1}, color);
    // upper right
    ctx.bl_ctx.fillRect(BLRect {x2 - d, y1, d + 1, 0 + 1}, color);
    ctx.bl_ctx.fillRect(BLRect {x2 + 0, y1, 0 + 1, d + 1}, color);
    // lower right
    ctx.bl_ctx.fillRect(BLRect {x2 - d, y2 + 0, d + 1, 0 + 1}, color);
    ctx.bl_ctx.fillRect(BLRect {x2 + 0, y2 - d, 0 + 1, d + 1}, color);
}

}  // namespace

auto render_circuit_overlay(Context& ctx, const WidgetRenderConfig& render_config) {
    if (render_config.show_render_borders) {
        draw_target_outline(ctx, 1, defaults::color_red);
        draw_target_outline(ctx, 0, defaults::color_lime);
    }
}

}  // namespace

auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config,
                       const Layout& layout) -> void {
    render_circuit_background(ctx);

    if (render_config.show_circuit) {
        // TODO write version that does not need surface ?
        render_layout(ctx, surface, layout);
    }

    render_circuit_overlay(ctx, render_config);
}

auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config,
                       const EditableCircuit& editable_circuit,
                       bool show_size_handles) -> void {
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
        render_layout_collision_index(ctx, editable_circuit);
    }
    if (render_config.show_connection_cache) {
        render_layout_connection_index(ctx, editable_circuit);
    }
    if (render_config.show_selection_cache) {
        render_layout_selection_index(ctx, editable_circuit);
    }

    render_circuit_overlay(ctx, render_config);
}

auto render_to_context(Context& ctx, const WidgetRenderConfig& render_config,
                       const SpatialSimulation& spatial_simulation) -> void {
    render_circuit_background(ctx);

    if (render_config.show_circuit) {
        render_simulation(ctx, spatial_simulation);
    }

    render_circuit_overlay(ctx, render_config);
}

namespace {

auto render_mouse_info_position(Context& ctx, const MousePositionInfo& info) -> void {
    const auto line_color = defaults::color_red;

    const auto pos = to_context(info.position, ctx);
    const auto size = ctx.settings.view_config.size();

    const auto w = gsl::narrow<double>(size.w);
    const auto h = gsl::narrow<double>(size.h);

    // cross
    ctx.bl_ctx.fillRect(BLRect {pos.x, 0, 1, h}, line_color);
    ctx.bl_ctx.fillRect(BLRect {0, pos.y, w, 1}, line_color);
}

auto render_mouse_info_labels(Context& ctx, const MousePositionInfo& info) -> void {
    const auto font_size = 16.0f;
    const auto attrs = TextCache::TextAttributes {.style = FontStyle::monospace};

    const auto pos = to_context(info.position, ctx);
    const auto text_x = pos.x + 20.0;
    const auto text_y = pos.y + font_size;

    auto draw_label = [&, y_ = text_y](std::string_view label_) mutable {
        ctx.cache.text_cache().draw_text(ctx.bl_ctx, BLPoint {text_x, y_}, label_,
                                         font_size, attrs);
        y_ += font_size;
    };

    for (const auto& label : info.labels) {
        draw_label(label);
    }
    draw_label(mouse_position_label("context", "BLPoint", pos));
    draw_label(mouse_position_label("grid", "point_fine_t",
                                    to_grid_fine(info.position, ctx.view_config())));
}

}  // namespace

auto render_mouse_position_info(Context& ctx, const WidgetRenderConfig& render_config,
                                const std::optional<MousePositionInfo>& info) -> void {
    if (info.has_value() && render_config.show_mouse_position) {
        render_mouse_info_position(ctx, *info);
        render_mouse_info_labels(ctx, *info);
    }
}

}  // namespace circuit_widget

}  // namespace logicsim
