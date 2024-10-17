#include "render/bl_error_check.h"

#include "algorithm/round.h"
#include "algorithm/to_underlying.h"

#include <blend2d.h>
#include <fmt/core.h>

#include <stdexcept>

namespace logicsim {

auto check_errors(const BLContext& ctx) -> void {
    const auto flag = ctx.accumulatedErrorFlags();

    if (flag != BL_CONTEXT_ERROR_NO_FLAGS) [[unlikely]] {
        const auto msg = fmt::format("Error in BLContext {}", to_underlying(flag));
        throw std::runtime_error(msg);
    }
}

auto ensure_all_saves_restored(const BLContext& ctx) -> void {
    if (ctx.savedStateCount() != 0) {
        throw std::runtime_error("Context has saved state at sync");
    }
}

auto checked_sync(BLContext& ctx) -> void {
    ensure_all_saves_restored(ctx);

    if (ctx.flush(BL_CONTEXT_FLUSH_SYNC) != BL_SUCCESS) {
        throw std::runtime_error("Error when calling BLContext::flush");
    };

    check_errors(ctx);
}

}  // namespace logicsim
