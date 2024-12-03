#include "core/random/fuzz.h"

#include "core/format/container.h"

namespace logicsim {

//
// Fuzz Stream
//

FuzzStream::FuzzStream(std::span<const uint8_t> data) : data_ {data} {}

auto FuzzStream::empty() const -> bool {
    return data_.empty();
}

auto FuzzStream::format() const -> std::string {
    return fmt::format("{}", data_);
}

auto FuzzStream::pop_or(uint8_t default_value) -> uint8_t {
    if (data_.empty()) {
        return default_value;
    }
    const auto result = data_[0];
    data_ = data_.subspan(std::size_t {1}, data_.size() - std::size_t {1});
    return result;
}

//
// Free Functions
//

auto fuzz_small_int(FuzzStream& stream, int lower, int upper) -> int {
    Expects(lower <= upper);
    if (lower == upper) {
        return lower;
    }

    // TODO add safe numerics
    const auto range = upper - lower;

    const auto offset = [range, &stream]() -> FuzzStream::value_type {
        Expects(range <= stream.max());
        static_assert(FuzzStream::min() == 0);

        if (range == FuzzStream::max()) {
            return stream.pop_or();
        }
        return stream.pop_or() % static_cast<FuzzStream::value_type>(range + 1);
    }();

    return lower + int {offset};
}

auto fuzz_bool(FuzzStream& stream) -> bool {
    return fuzz_small_int(stream, 0, 1) == 1;
}

}  // namespace logicsim
