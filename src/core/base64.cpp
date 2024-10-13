#include "base64.h"

#include <folly/base64.h>

namespace logicsim {

auto base64_encode(const std::string& data) -> std::string {
    return folly::base64Encode(data);
}

auto base64_decode(const std::string& data) -> tl::expected<std::string, LoadError> {
    auto result = std::string {};
    const auto result_size = folly::base64DecodedSize(data);
    folly::resizeWithoutInitialization(result, result_size);

    if (!folly::base64DecodeRuntime(data, result.data()).is_success) {
        return tl::unexpected<LoadError> {
            LoadErrorType::base64_decode_error,
            "Base64 Decoding failed",
        };
    }

    return result;
}

}  // namespace logicsim
