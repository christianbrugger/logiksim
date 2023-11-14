#ifndef LOGICSIM_RENDER_LAYER_SURFACE_H
#define LOGICSIM_RENDER_LAYER_SURFACE_H

#include "render/context.h"

#include <functional>

struct BLRectI;

namespace logicsim {

struct ContextRenderConfig;
class ContextGuard;

/**
 * brief: Defines a separate render layer with its own image data.
 */
struct LayerSurface {
    bool enabled {true};
    Context ctx {};

    auto initialize(const ContextRenderConfig& config) -> void;

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

/**
 * @brief: Renders the function first to the surface and then to the target within
 * the given dirty_rect.
 *
 * This function initializes the layer as needed with the settings of the target.
 * The function is rendered directly on the target, if the layer not enabled.
 *
 * Note that the passed bool indicates if the layer is used. This can be used
 * to set render with different blend modes.
 */
auto render_to_layer(Context& target_ctx, LayerSurface& surface, BLRectI dirty_rect,
                     std::function<void(Context&, bool)> render_func) -> void;

[[nodiscard]] auto make_context_guard(LayerSurface& surface) -> ContextGuard;

}  // namespace logicsim

#endif
