#include "render/bl_error_check.h"

#include <blend2d.h>
#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

auto check_errors(const BLContext& ctx) -> void {
    if (ctx.accumulatedErrorFlags() != BL_CONTEXT_ERROR_NO_FLAGS) [[unlikely]] {
        throw std::runtime_error(
            fmt::format("Error in BLContext {}", uint32_t {ctx.accumulatedErrorFlags()})
                .c_str());
    }
}

auto checked_sync(BLContext& ctx) -> void {
    if (ctx.savedStateCount() != 0) {
        throw std::runtime_error("context has saved state at sync");
    }

    ctx.flush(BL_CONTEXT_FLUSH_SYNC);
    check_errors(ctx);
}

}  // namespace logicsim
