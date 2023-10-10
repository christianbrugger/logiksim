#ifndef LOGICSIM_RENDER_PRIMITIVE_TEXT_H
#define LOGICSIM_RENDER_PRIMITIVE_TEXT_H

#include "vocabulary/color.h"
#include "vocabulary/font_style.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/text_alignment.h"

#include <string_view>

namespace logicsim {

struct point_fine_t;
struct Context;

struct TextAttributes {
    grid_fine_t font_size {1.0};
    color_t color {defaults::color_black};

    HTextAlignment horizontal_alignment {HTextAlignment::left};
    VTextAlignment vertical_alignment {VTextAlignment::baseline};
    FontStyle style {FontStyle::regular};

    // don't render, if scaled font size is smaller
    double cuttoff_size_px {3.0};  // pixels
};

auto draw_text(Context& ctx, point_fine_t position, std::string_view text,
               TextAttributes attributes) -> void;

}  // namespace logicsim

#endif
