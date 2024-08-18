#include "vocabulary/part.h"

#include <fmt/core.h>

namespace logicsim {

auto part_t::format() const -> std::string {
    return fmt::format("<part {}-{}>", begin, end);
}

}  // namespace logicsim
