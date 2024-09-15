#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_CIRCUIT_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_CIRCUIT_H

#include <filesystem>

namespace logicsim {

class Layout;
class Selection;

class SpatialSimulation;

struct ContextRenderSettings;
struct Context;
class ImageSurface;
class ContextCache;

//
// Layout
//

auto render_layout(Context& ctx, ImageSurface& surface, const Layout& layout) -> void;

auto render_layout(Context& ctx, ImageSurface& surface, const Layout& layout,
                   const Selection& selection) -> void;

/**
 * @brief: Render the layout to the given PNG file.
 *
 * Note, if fonts are required, a cache with loaded fonts is needed. E.g:
 *       const auto cache = cache_with_default_fonts();
 *   or
 *       const thread_local auto cache = cache_with_default_fonts();
 */
auto render_layout_to_file(const Layout& layout, const std::filesystem::path& filename,
                           const ContextRenderSettings& settings,
                           ContextCache cache) -> void;

auto render_layout_to_file(const Layout& layout, const Selection& selection,
                           const std::filesystem::path& filename,
                           const ContextRenderSettings& settings,
                           ContextCache cache) -> void;

//
// Simulation
//

auto render_simulation(Context& ctx, const SpatialSimulation& spatial_simulation) -> void;

auto render_simulation_to_file(const SpatialSimulation& spatial_simulation,
                               const std::filesystem::path& filename,
                               const ContextRenderSettings& settings,
                               ContextCache cache) -> void;

}  // namespace logicsim

#endif
