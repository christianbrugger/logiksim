#include "render/bl_error_check.h"

#include "algorithm/round.h"

#include <blend2d.h>
#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

auto check_errors(const BLContext& ctx) -> void {
    if (ctx.accumulatedErrorFlags() != BL_CONTEXT_ERROR_NO_FLAGS) [[unlikely]] {
        // TODO is this a lifetime issue?
        throw std::runtime_error(
            fmt::format("Error in BLContext {}", uint32_t {ctx.accumulatedErrorFlags()})
                .c_str());
    }
}

auto ensure_all_saves_restored(const BLContext& ctx) -> void {
    if (ctx.savedStateCount() != 0) {
        throw std::runtime_error("context has saved state at sync");
    }
}

auto checked_sync(BLContext& ctx) -> void {
    ensure_all_saves_restored(ctx);

    if (ctx.flush(BL_CONTEXT_FLUSH_SYNC) != BL_SUCCESS) {
        throw std::runtime_error("Error when calling BLContext::flush");
    };

    check_errors(ctx);
}

auto checked_end(BLContext& ctx) -> void {
    ensure_all_saves_restored(ctx);

    // ctx.end() frees the object state, so we need to check for errors before.
    try {
        check_errors(ctx);
    } catch (...) {
        ctx.end();
        throw;
    }

    if (ctx.end() != BL_SUCCESS) {
        throw std::runtime_error("Error when calling BLContext::end");
    };
}

}  // namespace logicsim
