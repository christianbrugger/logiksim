#ifndef LOGICSIM_RENDER_CONTEXT_H
#define LOGICSIM_RENDER_CONTEXT_H

#include "render/svg_cache.h"
#include "render/text_cache.h"
#include "vocabulary/context_render_config.h"

#include <blend2d.h>

namespace logicsim {

class ContextGuard;

/**
 * @brief: The context we pass to all of our render methods.
 */
struct Context {
    BLImage bl_image {};
    BLContext bl_ctx {};
    ContextRenderSettings settings {};
    TextCache text_cache {FontFaces {get_default_font_locations()}};
    SVGCache svg_cache {};

    auto begin() -> void;
    auto begin(BLImage& image) -> void;
    auto sync() -> void;
    auto end() -> void;

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

[[nodiscard]] auto make_context_guard(Context& ctx) -> ContextGuard;

// short-hand scene geometry that forwards config.view_config
[[nodiscard]] auto to_context(point_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(point_fine_t position, const Context& context) -> BLPoint;
[[nodiscard]] auto to_context(grid_t length, const Context& context) -> double;
[[nodiscard]] auto to_context(grid_fine_t length, const Context& context) -> double;
[[nodiscard]] auto to_context_unrounded(grid_fine_t length, const Context& context)
    -> double;

}  // namespace logicsim

#endif
