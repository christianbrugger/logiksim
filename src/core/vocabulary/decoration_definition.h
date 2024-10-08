#ifndef LOGICSIM_CORE_VOCABULARY_DECORATION_DEFINITION_H
#define LOGICSIM_CORE_VOCABULARY_DECORATION_DEFINITION_H

#include "format/struct.h"
#include "vocabulary/decoration_type.h"
#include "vocabulary/offset.h"

#include <compare>
#include <optional>
#include <string>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Text element specific attributes.
 */
struct attributes_text_element_t {
    std::string text {};
    offset_t width {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto operator==(const attributes_text_element_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const attributes_text_element_t&) const = default;
};

static_assert(std::is_aggregate_v<attributes_text_element_t>);

/**
 * @brief: Defines all attributes of a layout decoration.
 */
struct DecorationDefinition {
    DecorationType decoration_type {DecorationType::text_element};

    std::optional<attributes_text_element_t> attrs_text_element {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DecorationDefinition& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const DecorationDefinition& other) const = default;
};

static_assert(std::is_aggregate_v<DecorationDefinition>);

}  // namespace logicsim

#endif
