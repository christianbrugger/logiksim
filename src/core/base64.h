#ifndef LOGICSIM_BASE64_H
#define LOGICSIM_BASE64_H

#include <string>

namespace logicsim {

auto base64_encode(const std::string& data) -> std::string;
auto base64_decode(const std::string& data) -> std::string;

}  // namespace logicsim

#endif
