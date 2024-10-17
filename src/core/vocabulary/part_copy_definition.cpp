#include "core/vocabulary/part_copy_definition.h"

#include <fmt/core.h>

namespace logicsim {

auto part_copy_definition_t::format() const -> std::string {
    return fmt::format("<part_copy_definition destination = {}, source = {}>",
                       destination, source);
}

}  // namespace logicsim
