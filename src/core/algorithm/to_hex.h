#ifndef LOGICSIM_ALGORITHM_TO_HEX_H
#define LOGICSIM_ALGORITHM_TO_HEX_H

#include <string>
#include <string_view>

namespace logicsim {

[[nodiscard]] auto to_hex(std::string_view text) -> std::string;

[[nodiscard]] auto to_hex(std::wstring_view text) -> std::string;

}  // namespace logicsim

#endif
