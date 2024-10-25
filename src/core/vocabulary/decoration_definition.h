#ifndef LOGICSIM_CORE_VOCABULARY_DECORATION_DEFINITION_H
#define LOGICSIM_CORE_VOCABULARY_DECORATION_DEFINITION_H

#include "core/format/struct.h"
#include "core/vocabulary/decoration_type.h"
#include "core/vocabulary/font_style.h"
#include "core/vocabulary/size_2d.h"
#include "core/vocabulary/text_alignment.h"

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
    HTextAlignment horizontal_alignment {HTextAlignment::center};
    FontStyle font_style {FontStyle::regular};

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
    size_2d_t size {0, 0};

    std::optional<attributes_text_element_t> attrs_text_element {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const DecorationDefinition& other) const -> bool =
                                                                                  default;
    [[nodiscard]] auto operator<=>(const DecorationDefinition& other) const = default;
};

static_assert(std::is_aggregate_v<DecorationDefinition>);

}  // namespace logicsim

#endif
