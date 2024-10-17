#ifndef LOGICSIM_VOCABULARY_ALIGNMENT_H
#define LOGICSIM_VOCABULARY_ALIGNMENT_H

#include "core/format/enum.h"

#include <cstdint>
#include <string>

namespace logicsim {

enum class HorizontalAlignment : uint8_t {
    left,
    right,
    center,
};

template <>
[[nodiscard]] auto format(HorizontalAlignment alignment) -> std::string;

enum class VerticalAlignment : uint8_t {
    center,
    top,
    bottom,
};

template <>
[[nodiscard]] auto format(VerticalAlignment alignment) -> std::string;

}  // namespace logicsim

#endif
