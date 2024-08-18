#ifndef LOGICSIM_GZIP_H
#define LOGICSIM_GZIP_H

#include <string>

namespace logicsim {

auto gzip_compress(const std::string& input) -> std::string;
auto gzip_decompress(const std::string& input) -> std::string;

}  // namespace logicsim

#endif
