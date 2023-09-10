#include "render_helper.h"

namespace logicsim {

ContextGuard::ContextGuard(BLContext& bl_ctx) : bl_ctx_ {bl_ctx} {
    bl_ctx_.save();
}

ContextGuard::~ContextGuard() {
    bl_ctx_.restore();
};

auto make_context_guard(BLContext& bl_ctx) -> ContextGuard {
    return ContextGuard {bl_ctx};
}

}  // namespace logicsim