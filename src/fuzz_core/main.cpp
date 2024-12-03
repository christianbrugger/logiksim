
#include "core/component/editable_circuit/modifier.h"
#include "core/logging.h"
#include "core/random/fuzz.h"

#include <cstdint>
#include <span>

namespace logicsim {

namespace editable_circuit {

auto parse_message(FuzzStream& stream, Modifier& modifier) -> void {
    const bool horizontal = fuzz_bool(stream);

    const auto a = fuzz_small_int(stream, 0, 4);
    const auto b = fuzz_small_int(stream, 1, 5 - a);
    const auto c = fuzz_small_int(stream, 0, 5);

    const auto start = a;
    const auto end = a + b;
    const auto pos = c;

    const auto line = horizontal
                          ? ordered_line_t {point_t {start, pos}, point_t {end, pos}}
                          : ordered_line_t {point_t {pos, start}, point_t {pos, end}};

    const auto mode = [&stream] {
        switch (fuzz_small_int(stream, 0, 2)) {
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
    const auto span = std::span<const uint8_t> {data, size};

    process_data(span);

    return 0;
}
