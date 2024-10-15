#include "vocabulary/load_error.h"

#include <exception>

namespace logicsim {

template <>
auto format(LoadErrorType type) -> std::string {
    switch (type) {
        using enum LoadErrorType;

        case file_open_error:
            return "file_open_error";

        case unknown_file_format_error:
            return "unknown_file_format_error";
        case base64_decode_error:
            return "base64_decode_error";
        case gzip_decompress_error:
            return "gzip_decompress_error";
        case json_parse_error:
            return "json_parse_error";
        case json_version_error:
            return "json_version_error";
    }
    std::terminate();
}

LoadError::LoadError(LoadErrorType type, std::string message)
    : message_ {std::move(message)}, type_ {type} {}

auto LoadError::format() const -> const std::string& {
    return message_;
}

auto LoadError::type() const -> LoadErrorType {
    return type_;
}

}  // namespace logicsim
