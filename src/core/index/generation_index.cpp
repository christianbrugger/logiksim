#include "core/index/generation_index.h"

namespace logicsim {

GenerationIndex::GenerationIndex(const Layout& layout)
    : inputs {layout}, outputs {layout} {}

auto GenerationIndex::format() const -> std::string {
    return fmt::format(
        "GenerationIndex(\n"  //
        "  inputs = {}\n"     //
        "  outputs = {}\n"    //
        ")",
        inputs, outputs);
}

}  // namespace logicsim
