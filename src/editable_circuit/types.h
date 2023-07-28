#ifndef LOGIKSIM_EDITABLE_CIRCUIT_TYPES_H
#define LOGIKSIM_EDITABLE_CIRCUIT_TYPES_H

#include "format.h"
#include "vocabulary.h"

namespace logicsim {

enum class LineSegmentType {
    horizontal_first,
    vertical_first,
};

template <>
[[nodiscard]] auto format(LineSegmentType type) -> std::string;

struct LogicItemDefinition {
    ElementType element_type {ElementType::or_element};
    std::size_t input_count {3};
    std::size_t output_count {1};
    orientation_t orientation {orientation_t::right};
    logic_small_vector_t input_inverters {};
    logic_small_vector_t output_inverters {};

    [[nodiscard]] auto is_valid() const -> bool;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const LogicItemDefinition& other) const -> bool
        = default;
};

}  // namespace logicsim

#endif
