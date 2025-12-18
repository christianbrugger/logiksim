#include "core/render/circuit/render_circuit.h"

#include "core/geometry/scene.h"
#include "core/render/circuit/render_background.h"
#include "core/render/circuit/render_interactive_layer.h"
#include "core/render/circuit/render_simulation_layer.h"
#include "core/render/context.h"
#include "core/render/image_surface.h"
#include "core/render/render_context.h"
#include "core/selection.h"
#include "core/spatial_simulation.h"
#include "core/vocabulary/context_render_settings.h"
#include "core/vocabulary/rect.h"

namespace logicsim {

auto _render_layout(Context& ctx, ImageSurface& surface, const Layout& layout,
                    const Selection* selection) -> void {
    const auto scene_rect = get_scene_rect(ctx.settings.view_config);
    const auto layers = build_interactive_layers(layout, selection, scene_rect);

    render_interactive_layers(ctx, layout, layers, surface);
}

auto render_layout(Context& ctx, ImageSurface& surface, const Layout& layout) -> void {
    _render_layout(ctx, surface, layout, nullptr);
}

auto render_layout(Context& ctx, ImageSurface& surface, const Layout& layout,
                   const Selection& selection) -> void {
    if (selection.empty()) {
        _render_layout(ctx, surface, layout, nullptr);
    } else {
        _render_layout(ctx, surface, layout, &selection);
    }
}

auto render_layout_to_file(const Layout& layout, const std::filesystem::path& filename,
                           const ContextRenderSettings& settings, ContextCache cache)
    -> void {
    // allocation time is small compared to encoding time, so we allocate it here
    auto surface = ImageSurface {};

    render_to_file(filename, settings, std::move(cache), [&](Context& ctx) {
        render_background(ctx);
        render_layout(ctx, surface, layout);
    });
}

auto render_layout_to_file(const Layout& layout, const Selection& selection,
                           const std::filesystem::path& filename,
                           const ContextRenderSettings& settings, ContextCache cache)
    -> void {
    // allocation time is small compared to encoding time, so we allocate it here
    auto surface = ImageSurface {};

    render_to_file(filename, settings, std::move(cache), [&](Context& ctx) {
        render_background(ctx);
        render_layout(ctx, surface, layout, selection);
    });
}

//
// Simulation
//

auto render_simulation(Context& ctx, const SpatialSimulation& spatial_simulation)
    -> void {
    const auto scene_rect = get_scene_rect(ctx.view_config());
    const auto layers = build_simulation_layers(spatial_simulation.layout(), scene_rect);

    render_simulation_layers(ctx, spatial_simulation, layers);
}

auto render_simulation_to_file(const SpatialSimulation& spatial_simulation,
                               const std::filesystem::path& filename,
                               const ContextRenderSettings& settings, ContextCache cache)
    -> void {
    render_to_file(filename, settings, std::move(cache), [&](Context& ctx) {
        render_background(ctx);
        render_simulation(ctx, spatial_simulation);
    });
}

}  // namespace logicsim
