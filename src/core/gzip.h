#ifndef LOGICSIM_GZIP_H
#define LOGICSIM_GZIP_H

#include "vocabulary/load_error.h"

#include <tl/expected.hpp>

#include <string>

namespace logicsim {

auto gzip_compress(const std::string& input) -> std::string;
auto gzip_decompress(const std::string& input) -> tl::expected<std::string, LoadError>;

}  // namespace logicsim

#endif
