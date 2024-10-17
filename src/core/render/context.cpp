#include "core/render/context.h"

#include "core/geometry/scene.h"
#include "core/render/context_guard.h"
#include "core/vocabulary/point_device.h"
#include "core/vocabulary/point_device_fine.h"

namespace logicsim {

//
// Context
//

auto Context::view_config() const -> const ViewConfig& {
    return settings.view_config;
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

auto to_context(point_device_fine_t position, const Context& context) -> BLPoint {
    return to_context(position, context.settings.view_config);
}

auto to_context(point_device_t position, const Context& context) -> BLPoint {
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

//
//
//

}  // namespace logicsim
