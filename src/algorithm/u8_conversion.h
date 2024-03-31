#ifndef LOGICSIM_ALGORITHM_U8_CONVERSION_H
#define LOGICSIM_ALGORITHM_U8_CONVERSION_H

#include <string_view>

namespace logicsim {

[[nodiscard]] auto to_u8string(std::string_view filename) -> std::u8string;

}

#endif
