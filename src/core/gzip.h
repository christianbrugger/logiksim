#ifndef LOGICSIM_GZIP_H
#define LOGICSIM_GZIP_H

#include "core/vocabulary/load_error.h"

#include <tl/expected.hpp>

#include <string_view>

namespace logicsim {

auto gzip_compress(std::string_view input) -> std::string;
auto gzip_decompress(std::string_view input) -> tl::expected<std::string, LoadError>;

}  // namespace logicsim

#endif
