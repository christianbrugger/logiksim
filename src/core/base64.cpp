#include "base64.h"

#include "logging.h"

#include <cppcodec/base64_rfc4648.hpp>

namespace logicsim {

auto base64_encode(const std::string& data) -> std::string {
    return cppcodec::base64_rfc4648::encode(data);
}

auto base64_decode(const std::string& data) -> tl::expected<std::string, LoadError> {
    try {
        return cppcodec::base64_rfc4648::decode<std::string>(data);
    } catch (const cppcodec::parse_error& error) {
        return tl::unexpected {LoadError {std::string {error.what()}}};
    }
}

}  // namespace logicsim
