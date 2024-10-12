#ifndef LOGICSIM_BASE64_H
#define LOGICSIM_BASE64_H

#include "vocabulary/load_error.h"

#include <tl/expected.hpp>

#include <string>

namespace logicsim {

auto base64_encode(const std::string& data) -> std::string;
auto base64_decode(const std::string& data) -> tl::expected<std::string, LoadError>;

}  // namespace logicsim

#endif
