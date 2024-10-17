#ifndef LOGICSIM_VOCABULARY_TEXT_ALIGNMENT_H
#define LOGICSIM_VOCABULARY_TEXT_ALIGNMENT_H

#include "core/format/enum.h"

#include <cstdint>
#include <string>

namespace logicsim {

enum class HTextAlignment : uint8_t {
    left,
    right,
    center,
};

template <>
[[nodiscard]] auto format(HTextAlignment alignment) -> std::string;

enum class VTextAlignment : uint8_t {
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
[[nodiscard]] auto format(VTextAlignment alignment) -> std::string;

}  // namespace logicsim

#endif
