#include "algorithm/to_hex.h"

#include "algorithm/fmt_join.h"

#include <fmt/xchar.h>

#include <cstdint>
#include <ranges>

namespace logicsim {

auto to_hex(std::string_view text) -> std::string {
    const auto rng = std::ranges::subrange(text.begin(), text.end());
    return fmt_join("", rng, "\\x{:x}", [](char c) {
        static_assert(sizeof(c) <= sizeof(uint8_t));
        return static_cast<uint8_t>(c);
    });
}

auto to_hex(std::wstring_view text) -> std::string {
    auto result = std::string {};

    for (const auto c : text) {
        static_assert(sizeof(c) <= sizeof(uint32_t));
        result += fmt::format("\\u{:0{}x}", static_cast<uint32_t>(c), sizeof(c) * 2);
    }

    return result;
}

}  // namespace logicsim
