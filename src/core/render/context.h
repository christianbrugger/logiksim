#ifndef LOGICSIM_RENDER_CONTEXT2_H
#define LOGICSIM_RENDER_CONTEXT2_H

#include "render/context_cache.h"
#include "vocabulary/context_render_config.h"

#include <blend2d.h>

namespace logicsim {

class ContextGuard;
struct point_device_t;
struct point_device_fine_t;

/**
 * @brief: Generic render context that render code operates on for one frame.
 */
struct Context {
    BLContext bl_ctx {};
    ContextRenderSettings settings {};
    ContextCache cache {};

    [[nodiscard]] auto view_config() const -> const ViewConfig&;
};

[[nodiscard]] auto make_context_guard(Context& ctx) -> ContextGuard;

// short-hand scene geometry that forwards config.view_config
[[nodiscard]] auto to_context(point_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(point_fine_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(point_device_fine_t position,
                              const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(point_device_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(grid_t length, const Context& context) -> double;
[[nodiscard]] auto to_context(grid_fine_t length, const Context& context) -> double;
[[nodiscard]] auto to_context_unrounded(grid_fine_t length,
                                        const Context& context) -> double;

}  // namespace logicsim

#endif
