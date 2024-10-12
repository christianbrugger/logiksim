#include "vocabulary/save_format.h"

#include <exception>

namespace logicsim {

template <>
auto format(SaveFormat format) -> std::string {
    switch (format) {
        using enum SaveFormat;

        case base64_gzip:
            return "base64_gzip";
        case gzip:
            return "gzip";
        case json:
            return "json";
    }
    std::terminate();
}

[[nodiscard]] auto guess_save_format(std::string_view binary)
    -> std::optional<SaveFormat> {
    // detect json that starts with list or dict
    if (binary.size() >= 1 && (binary.at(0) == '{' || binary.at(0) == '[')) {
        return SaveFormat::json;
    }

    // detect gzip header
    if (binary.size() >= 3 &&                                //
        binary.at(0) == '\x1F' && binary.at(1) == '\x8B' &&  // magic number
        binary.at(2) == '\x08'                               // defalte compression method
    ) {
        return SaveFormat::gzip;
    }

    // detect base64 encoded gzip header "\x1F\x8B\x08" = "H4sI"
    if (binary.size() >= 4 &&   //
        binary.at(0) == 'H' &&  //
        binary.at(1) == '4' &&  //
        binary.at(2) == 's' &&  //
        binary.at(3) == 'I') {
        return SaveFormat::base64_gzip;
    }

    return std::nullopt;
}

}  // namespace logicsim
