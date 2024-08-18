#include "base64.h"

#include "logging.h"

#include <cppcodec/base64_rfc4648.hpp>

namespace logicsim {

auto base64_encode(const std::string& data) -> std::string {
    return cppcodec::base64_rfc4648::encode(data);
}

auto base64_decode(const std::string& data) -> std::string {
    try {
        return cppcodec::base64_rfc4648::decode<std::string>(data);
    } catch (cppcodec::parse_error&) {
        print("WARNING: Error during base64 decode");
    }
    return "";
}

}  // namespace logicsim
