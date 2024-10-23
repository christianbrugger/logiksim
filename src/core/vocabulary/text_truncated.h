#ifndef LOGICSIM_CORE_VOCABULARY_TEXT_TRUNCATED_H
#define LOGICSIM_CORE_VOCABULARY_TEXT_TRUNCATED_H

#include "core/format/enum.h"

namespace logicsim {

/**
 * @brief: yes, if text was not fully drawn, due to max_text_width.
 */
enum class TextTruncated {
    yes,
    no,
};

template <>
[[nodiscard]] auto format(TextTruncated value) -> std::string;

}  // namespace logicsim

#endif
