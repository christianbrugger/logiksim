#ifndef LOGICSIM_VOCABULARY_TIME_RATE_H
#define LOGICSIM_VOCABULARY_TIME_RATE_H

#include "format/struct.h"
#include "vocabulary/time.h"
#include "vocabulary/time_literal.h"

#include <compare>

namespace logicsim {

struct time_rate_t {
    time_t rate_per_second;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_rate_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_rate_t &other) const = default;
};

}  // namespace logicsim

#endif
