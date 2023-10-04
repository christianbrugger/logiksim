#ifndef LOGIKSIM_EDITABLE_CIRCUIT_TYPE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_TYPE_H

#include "format/enum.h"
#include "format/struct.h"
#include "layout.h"
#include "vocabulary.h"

#include <cstddef>
#include <optional>

namespace logicsim {

enum class LineInsertionType {
    horizontal_first,
    vertical_first,
};

template <>
[[nodiscard]] auto format(LineInsertionType type) -> std::string;

struct LogicItemDefinition {
    ElementType element_type {ElementType::or_element};
    connection_count_t input_count {3};
    connection_count_t output_count {1};
    orientation_t orientation {orientation_t::right};
    logic_small_vector_t input_inverters {};
    logic_small_vector_t output_inverters {};

    std::optional<layout::attributes_clock_generator> attrs_clock_generator {};

    [[nodiscard]] auto is_valid() const -> bool;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const LogicItemDefinition &other) const
        -> bool = default;
};

}  // namespace logicsim

#endif
