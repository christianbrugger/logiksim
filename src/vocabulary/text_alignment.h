#ifndef LOGICSIM_VOCABULARY_TEXT_ALIGNMENT_H
#define LOGICSIM_VOCABULARY_TEXT_ALIGNMENT_H

#include "format/enum.h"

#include <cstdint>
#include <string>

namespace logicsim {

enum class HorizontalAlignment : uint8_t {
    left,
    right,
    center,
};

template <>
auto format(HorizontalAlignment alignment) -> std::string;

enum class VerticalAlignment : uint8_t {
    baseline,
    // adjusts the baseline
    center_baseline,
    top_baseline,
    bottom_baseline,
    // aligns the specific text
    center,
    top,
    bottom,
};

template <>
auto format(VerticalAlignment alignment) -> std::string;

}  // namespace logicsim

#endif
