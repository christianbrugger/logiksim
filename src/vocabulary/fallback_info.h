#ifndef LOGICSIM_VOCABULARY_FALLBACK_INFO_H
#define LOGICSIM_VOCABULARY_FALLBACK_INFO_H

#include "format/struct.h"

#include <concepts>
#include <string>
#include <string_view>

namespace logicsim {

struct fallback_info_t {
    std::string message {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const fallback_info_t&) const -> bool = default;
    [[nodiscard]] explicit operator bool() const;
};

static_assert(std::regular<fallback_info_t>);

/**
 * @brief: Prints fallback messages, suppressing repeated messages.
 */
class FallbackPrinter {
   public:
    auto print_if_set(std::string_view message, const fallback_info_t& info) -> void;

   private:
    fallback_info_t last_info_ {};
};

}  // namespace logicsim

#endif
