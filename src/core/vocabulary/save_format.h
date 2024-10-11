#ifndef LOGICSIM_CORE_VOCABULARY_SAVE_FORMAT_H
#define LOGICSIM_CORE_VOCABULARY_SAVE_FORMAT_H

#include "format/enum.h"

#include <string>

namespace logicsim {

enum SaveFormat {
    base64_gzip,
    gzip,
    json,
};

template <>
[[nodiscard]] auto format(SaveFormat format) -> std::string;

[[nodiscard]] auto guess_save_format(const std::string& binary)
    -> std::optional<SaveFormat>;

}  // namespace logicsim

#endif
