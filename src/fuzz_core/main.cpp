
#include "core/component/editable_circuit/modifier.h"
#include "core/format/struct.h"
#include "core/logging.h"

#include <cstdint>
#include <span>

namespace logicsim {

namespace editable_circuit {

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

FuzzStream::FuzzStream(std::span<const uint8_t> data) : data_ {data} {}

auto FuzzStream::empty() const -> bool {
    return data_.empty();
}

auto FuzzStream::format() const -> std::string {
    return fmt::format("{}", data_);
}

constexpr auto FuzzStream::min() -> uint8_t {
    return std::numeric_limits<uint8_t>::min();
}

constexpr auto FuzzStream::max() -> uint8_t {
    return std::numeric_limits<uint8_t>::max();
}

auto FuzzStream::pop_or(uint8_t default_value) -> uint8_t {
    if (data_.empty()) {
        return default_value;
    }
    const auto result = data_[0];
    data_ = data_.subspan(std::size_t {1}, data_.size() - std::size_t {1});
    return result;
}

[[nodiscard]] auto fuzz_int(FuzzStream& stream, int lower, int upper) -> int {
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

[[nodiscard]] auto fuzz_bool(FuzzStream& stream) -> bool {
    return fuzz_int(stream, 0, 1) == 1;
}

auto parse_message(FuzzStream& stream, Modifier& modifier) -> void {
    const bool horizontal = fuzz_bool(stream);

    const auto a = fuzz_int(stream, 0, 4);
    const auto b = fuzz_int(stream, 1, 5 - a);
    const auto c = fuzz_int(stream, 0, 5);

    const auto start = a;
    const auto end = a + b;
    const auto pos = c;

    print(start, end, pos);

    const auto line = horizontal
                          ? ordered_line_t {point_t {start, pos}, point_t {end, pos}}
                          : ordered_line_t {point_t {pos, start}, point_t {pos, end}};

    const auto mode = [&stream] {
        switch (fuzz_int(stream, 0, 2)) {
            case 0:
                return InsertionMode::insert_or_discard;
            case 1:
                return InsertionMode::collisions;
            case 2:
                return InsertionMode::temporary;
        }
        std::terminate();
    }();

    modifier.add_wire_segment(line, mode);
}

auto process_data(std::span<const uint8_t> data) -> void {
    auto stream = FuzzStream(data);

    auto modifier = Modifier {Layout {}, ModifierConfig {
                                             .enable_history = true,
                                             .validate_messages = true,
                                         }};

    while (!stream.empty()) {
        parse_message(stream, modifier);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) -> int {
    using namespace logicsim::editable_circuit;

    process_data(std::span<const uint8_t> {data, size});

    return 0;
}
