#ifndef LOGICSIM_VOCABULARY_SETTING_ATTRIBUTE_H
#define LOGICSIM_VOCABULARY_SETTING_ATTRIBUTE_H

#include <variant>

namespace logicsim {

struct attributes_clock_generator_t;
struct attributes_text_element_t;

using SettingAttributes =
    std::variant<attributes_clock_generator_t, attributes_text_element_t>;

}  // namespace logicsim

#endif
