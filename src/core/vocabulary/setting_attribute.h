#ifndef LOGICSIM_VOCABULARY_SETTING_ATTRIBUTE_H
#define LOGICSIM_VOCABULARY_SETTING_ATTRIBUTE_H

#include "format/struct.h"
#include "vocabulary/logicitem_definition.h"

#include <compare>
#include <optional>

namespace logicsim {

struct SettingAttributes {
    std::optional<attributes_clock_generator_t> attrs_clock_generator {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const SettingAttributes& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const SettingAttributes& other) const = default;
};

}  // namespace logicsim

#endif
