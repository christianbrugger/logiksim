#ifndef LOGICSIM_RENDER_CONTEXT2_H
#define LOGICSIM_RENDER_CONTEXT2_H

#include "render/svg_cache.h"
#include "render/text_cache.h"
#include "vocabulary/context_render_config.h"

#include <blend2d.h>

namespace logicsim {

struct Context {
    BLContext bl_ctx {};
    ContextRenderSettings settings {};
    TextCache text_cache {};
    SVGCache svg_cache {};

    auto clear() -> void;
    auto shrink_to_fit() -> void;
};

}  // namespace logicsim

#endif
