#ifndef LOGICSIM_CORE_RANDOM_FUZZ_H
#define LOGICSIM_CORE_RANDOM_FUZZ_H

#include "core/concept/integral.h"
#include "core/format/struct.h"

#include <gsl/gsl>

#include <cstdint>
#include <limits>
#include <span>

namespace logicsim {

class FuzzStream {
   public:
    using value_type = uint8_t;

    explicit FuzzStream() = default;
    explicit FuzzStream(std::span<const uint8_t> data);

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr static auto min() -> uint8_t;
    [[nodiscard]] constexpr static auto max() -> uint8_t;

    [[nodiscard]] auto pop_or(uint8_t default_value = 0) -> uint8_t;

   private:
    std::span<const uint8_t> data_ {};
};

//
// Free Functions
//

[[nodiscard]] auto clamp_to_fuzz_stream(integral auto number) -> FuzzStream::value_type;

[[nodiscard]] auto fuzz_small_int(FuzzStream& stream, int lower, int upper) -> int;
[[nodiscard]] auto fuzz_bool(FuzzStream& stream) -> bool;
[[nodiscard]] auto fuzz_double_inclusive(FuzzStream& stream, double lower,
                                         double upper) -> double;

//
// Implementation
//
constexpr auto FuzzStream::min() -> uint8_t {
    return std::numeric_limits<uint8_t>::min();
}

constexpr auto FuzzStream::max() -> uint8_t {
    return std::numeric_limits<uint8_t>::max();
}

auto clamp_to_fuzz_stream(integral auto number) -> FuzzStream::value_type {
    Expects(sizeof(number) <= sizeof(uint64_t));
    Expects(number >= decltype(number) {0});

    return gsl::narrow_cast<FuzzStream::value_type>(
        std::min(uint64_t {FuzzStream::max()}, gsl::narrow_cast<uint64_t>(number)));
}

}  // namespace logicsim

#endif
