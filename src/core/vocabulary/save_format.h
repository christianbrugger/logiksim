#ifndef LOGICSIM_CORE_VOCABULARY_SAVE_FORMAT_H
#define LOGICSIM_CORE_VOCABULARY_SAVE_FORMAT_H

#include "core/format/enum.h"

#include <optional>
#include <string>
#include <string_view>

namespace logicsim {

enum SaveFormat {
    base64_gzip,
    gzip,
    json,
};

template <>
[[nodiscard]] auto format(SaveFormat format) -> std::string;

[[nodiscard]] auto guess_save_format(std::string_view binary)
    -> std::optional<SaveFormat>;

}  // namespace logicsim

#endif
