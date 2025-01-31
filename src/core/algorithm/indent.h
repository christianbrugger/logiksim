#ifndef LOGICSIM_CORE_ALGORITHM_INDENT_H
#define LOGICSIM_CORE_ALGORITHM_INDENT_H

#include <string>
#include <string_view>

namespace logicsim {

[[nodiscard]] auto indent(std::string_view input, int count) -> std::string;

}  // namespace logicsim

#endif
