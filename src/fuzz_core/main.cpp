
#include "core/component/editable_circuit/modifier.h"
#include "core/logging.h"

#include <cstdint>
#include <span>

namespace logicsim {

namespace editable_circuit {

auto get_count(std::span<const uint8_t>& data,
               size_t count) -> std::optional<std::span<const uint8_t>> {
    if (data.size() < count) {
        data = {};
        return std::nullopt;
    }

    const auto front = data.subspan(0, count);
    data = data.subspan(count, data.size() - count);
    return front;
}

auto at(std::span<const uint8_t> value, size_t index) -> uint8_t {
    if (index >= value.size()) {
        throw std::runtime_error("index out of range");
    }
    return value[index];
}

auto parse_message(std::span<const uint8_t>& data, Modifier& modifier) -> void {
    if (const auto value = get_count(data, 5)) {
        const bool horizontal = (at(*value, 0) % 2) == 0;

        const auto a = at(*value, 1) % 5;
        const auto b = at(*value, 2) % (5 - a) + 1;
        const auto c = at(*value, 3) % 6;

        const auto start = a;
        const auto end = a + b;
        const auto pos = c;

        const auto line = horizontal
                              ? ordered_line_t {point_t {start, pos}, point_t {end, pos}}
                              : ordered_line_t {point_t {pos, start}, point_t {pos, end}};

        const auto mode = [=] {
            switch (at(*value, 4) % 3) {
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
}

auto process_data(std::span<const uint8_t> data) -> void {
    auto modifier = Modifier {Layout {}, ModifierConfig {
                                             .enable_history = true,
                                             .validate_messages = true,
                                         }};

    while (!data.empty()) {
        parse_message(data, modifier);
    }
    // print(modifier.circuit_data().layout);
}

}  // namespace editable_circuit

}  // namespace logicsim

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) -> int {
    using namespace logicsim::editable_circuit;

    process_data(std::span<const uint8_t> {data, size});

    return 0;
}
