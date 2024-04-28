#include "render/context2.h"

#include "geometry/scene.h"
#include "render/context_guard.h"

namespace logicsim {

//
// Context Data
//

auto ContextData::clear() -> void {
    text_cache.clear();
    svg_cache.clear();
}

auto ContextData::shrink_to_fit() -> void {
    text_cache.shrink_to_fit();
    svg_cache.shrink_to_fit();
}

//
// Context
//

Context::Context(BLContext&& ctx, ContextData&& data)
    : data_ {std::move(data)}, bl_ctx {std::move(ctx)} {}

auto Context::data() const -> const ContextData& {
    return data_.value();
}

auto Context::extract_data() -> ContextData {
    return std::exchange(data_, std::nullopt).value();
}

//
// Free Functions
//

auto make_context_guard(Context& ctx) -> ContextGuard {
    return make_context_guard(ctx.bl_ctx);
}

auto to_context(point_t position, const Context& context) -> BLPoint {
    return to_context(position, context.data().settings.view_config);
}

auto to_context(point_fine_t position, const Context& context) -> BLPoint {
    return to_context(position, context.data().settings.view_config);
}

auto to_context(grid_t length, const Context& context) -> double {
    return to_context(length, context.data().settings.view_config);
}

auto to_context(grid_fine_t length, const Context& context) -> double {
    return to_context(length, context.data().settings.view_config);
}

auto to_context_unrounded(grid_fine_t length, const Context& context) -> double {
    return to_context_unrounded(length, context.data().settings.view_config);
}

}  // namespace logicsim
