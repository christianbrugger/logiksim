#ifndef LOGICSIM_VOCABULARY_ELEMENT_DRAW_STATE_H
#define LOGICSIM_VOCABULARY_ELEMENT_DRAW_STATE_H

#include "format/enum.h"

#include <cstdint>
#include <string>

namespace logicsim {

/**
 * @brief:
 */
enum class ElementDrawState : uint32_t {
    // inserted
    normal,
    normal_selected,
    valid,
    simulated,

    // uninserted
    colliding,
    temporary_selected,
};

template <>
auto format(ElementDrawState) -> std::string;

[[nodiscard]] auto is_inserted(ElementDrawState state) noexcept -> bool;

[[nodiscard]] auto has_overlay(ElementDrawState state) noexcept -> bool;

}  // namespace logicsim

#endif
