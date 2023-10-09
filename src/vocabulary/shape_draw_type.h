#ifndef LOGICSIM_VOCABULARY_SHAPE_DRAW_TYPE_H
#define LOGICSIM_VOCABULARY_SHAPE_DRAW_TYPE_H

#include "format/enum.h"

#include <string>

namespace logicsim {

enum class ShapeDrawType {
    fill,
    stroke,
    fill_and_stroke,
};

template <>
auto format(ShapeDrawType type) -> std::string;

[[nodiscard]] auto do_fill(ShapeDrawType type) -> bool;
[[nodiscard]] auto do_stroke(ShapeDrawType type) -> bool;

}  // namespace logicsim

#endif
