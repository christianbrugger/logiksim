#include "core/render/render_context.h"

#include "core/algorithm/round.h"
#include "core/render/context_guard.h"
#include "core/render/context_info.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

auto create_context(BLImage &bl_image, const ContextRenderSettings &render_settings)
    -> BLContext {
    if (bl_image.size() != render_settings.view_config.size()) {
        throw std::runtime_error("Given bl_image does not match size of settings.");
    }

    // our render code depends on rounding mode
    Expects(correct_round_mode());

    return BLContext {bl_image, context_info(render_settings)};
}

auto blit_layer(Context &target_ctx, const BLImage &source_image, BLRectI dirty_rect)
    -> void {
    if (target_ctx.bl_ctx.target_size() != source_image.size()) [[unlikely]] {
        throw std::runtime_error("target_ctx and source_image need to have same size.");
    }

    auto _ [[maybe_unused]] = make_context_guard(target_ctx);

    target_ctx.bl_ctx.set_comp_op(BL_COMP_OP_SRC_OVER);
    target_ctx.bl_ctx.blit_image(dirty_rect, source_image, dirty_rect);
}

}  // namespace logicsim
