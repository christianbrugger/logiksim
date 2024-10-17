#include "core/render/primitive/stroke.h"

#include "core/render/context.h"
#include "core/vocabulary/view_config.h"

namespace logicsim {

auto resolve_stroke_width(int attribute, const ViewConfig& view_config) -> int {
    return attribute == defaults::use_view_config_stroke_width
               ? view_config.stroke_width()
               : attribute;
}

auto resolve_stroke_width(int attribute, const Context& ctx) -> int {
    return resolve_stroke_width(attribute, ctx.view_config());
}

auto stroke_offset(int stroke_width) -> double {
    // To allign our strokes to the pixel grid, we need to offset odd strokes
    // otherwise they are drawn between pixels and get blurry
    if (stroke_width % 2 == 0) {
        return 0;
    }
    return 0.5;
}

}  // namespace logicsim
