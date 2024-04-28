#ifndef LOGICSIM_RENDER_CONTEXT2_H
#define LOGICSIM_RENDER_CONTEXT2_H

#include "render/svg_cache.h"
#include "render/text_cache.h"
#include "vocabulary/context_render_config.h"

#include <blend2d.h>

namespace logicsim {

class ContextGuard;

/**
 * @brief: Data and caches that are persistent across rendered frames.
 */
struct ContextData {
    ContextRenderSettings settings {};
    TextCache text_cache {};
    SVGCache svg_cache {};

    /**
     * @brief: clear cached data
     */
    auto clear() -> void;
    /**
     * @brief: shrinks unused memory allocations of caches
     */
    auto shrink_to_fit() -> void;
};

/**
 * @brief: Generic render context that most render code operates on for one frame.
 */
struct Context {
    explicit Context() = default;
    explicit Context(BLContext&& ctx, ContextData&& data);

    [[nodiscard]] auto data() const -> const ContextData&;
    [[nodiscard]] auto extract_data() -> ContextData;

   private:
    // data is encapsulated so it is read only
    // an optional is used so it throws when used after extraction
    std::optional<ContextData> data_ {};

   public:
    // context is public, as write access is required, getters not needed
    BLContext bl_ctx {};
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
