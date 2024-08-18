#include "format/to_hex.h"

#include "algorithm/fmt_join.h"

#include <cstdint>
#include <ranges>

namespace logicsim {

auto to_hex(std::string text) -> std::string {
    const auto rng = std::ranges::subrange(text.begin(), text.end());
    return fmt_join("", rng, "{:x}", [](char c) { return static_cast<uint8_t>(c); });
}

}  // namespace logicsim
