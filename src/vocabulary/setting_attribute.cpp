#include "vocabulary/setting_attribute.h"

#include "format/std_type.h"

namespace logicsim {

auto SettingAttributes::format() const -> std::string {
    return fmt::format("SettingAttributes(attrs_clock_generator = {})",
                       attrs_clock_generator);
}

}  // namespace logicsim
