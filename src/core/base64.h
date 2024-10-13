#ifndef LOGICSIM_BASE64_H
#define LOGICSIM_BASE64_H

#include "vocabulary/load_error.h"

#include <tl/expected.hpp>

#include <string_view>

namespace logicsim {

auto base64_encode(std::string_view data) -> std::string;
auto base64_decode(std::string_view data) -> tl::expected<std::string, LoadError>;

}  // namespace logicsim

#endif
