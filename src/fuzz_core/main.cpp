
#include <cstdint>
#include <exception>
#include <span>

namespace logicsim {

auto process_data(std::span<const uint8_t> data) -> void {
    if (data.size() > 20 && data[0] == 'H' && data[1] == 'e' && data[2] == 'l' &&
        data[3] == 'l' && data[4] == 'o') {
        std::terminate();
        return;
    }
}

}  // namespace logicsim

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) -> int {
    using namespace logicsim;

    process_data(std::span<const uint8_t> {data, size});

    return 0;
}
