#include "render/context.h"

#include "geometry/scene.h"
#include "render/bl_error_check.h"
#include "render/context_guard.h"
#include "render/context_info.h"

namespace logicsim {

auto Context::begin() -> void {
    settings.view_config.set_size(bl_image.size());
    bl_ctx.begin(bl_image, context_info(settings));
}

auto Context::sync() -> void {
    checked_sync(bl_ctx);
}

auto Context::end() -> void {
    bl_ctx.end();
    check_errors(bl_ctx);
}

auto Context::clear() -> void {
    text_cache.clear();
    svg_cache.clear();
}

auto Context::shrink_to_fit() -> void {
    text_cache.shrink_to_fit();
    svg_cache.shrink_to_fit();
}

//
// Free Functions
//

auto make_context_guard(Context& ctx) -> ContextGuard {
    return make_context_guard(ctx.bl_ctx);
}

auto to_context(point_t position, const Context& context) -> BLPoint {
    return to_context(position, context.settings.view_config);
}

auto to_context(point_fine_t position, const Context& context) -> BLPoint {
    return to_context(position, context.settings.view_config);
}

auto to_context(grid_t length, const Context& context) -> double {
    return to_context(length, context.settings.view_config);
}

auto to_context(grid_fine_t length, const Context& context) -> double {
    return to_context(length, context.settings.view_config);
}

auto to_context_unrounded(grid_fine_t length, const Context& context) -> double {
    return to_context_unrounded(length, context.settings.view_config);
}

}  // namespace logicsim
