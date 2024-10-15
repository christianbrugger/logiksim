#ifndef LOGICSIM_CORE_VOCABULARY_LOAD_ERROR_H
#define LOGICSIM_CORE_VOCABULARY_LOAD_ERROR_H

#include "format/enum.h"
#include "format/struct.h"

#include <string>

namespace logicsim {

enum class LoadErrorType {
    file_open_error,

    unknown_file_format_error,
    base64_decode_error,
    gzip_decompress_error,
    json_parse_error,
    json_version_error,
};

template <>
[[nodiscard]] auto format(LoadErrorType type) -> std::string;

/**
 * @brief: Error while loading file or deserializing data.
 */
class LoadError {
   public:
    explicit LoadError(LoadErrorType type, std::string message);

    [[nodiscard]] auto format() const -> const std::string&;
    [[nodiscard]] auto type() const -> LoadErrorType;

   private:
    std::string message_ {};
    LoadErrorType type_ {};
};

}  // namespace logicsim

#endif
