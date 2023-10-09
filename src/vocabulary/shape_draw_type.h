#ifndef LOGICSIM_VOCABULARY_SHAPE_DRAW_TYPE_H
#define LOGICSIM_VOCABULARY_SHAPE_DRAW_TYPE_H

#include "format/enum.h"

#include <string>

namespace logicsim {

enum class DrawType {
    fill,
    stroke,
    fill_and_stroke,
};

template <>
auto format(DrawType type) -> std::string;

[[nodiscard]] auto do_fill(DrawType type) -> bool;
[[nodiscard]] auto do_stroke(DrawType type) -> bool;

}  // namespace logicsim

#endif
