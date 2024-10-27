#ifndef LOGICSIM_CORE_INDEX_GENERATION_INDEX_H
#define LOGICSIM_CORE_INDEX_GENERATION_INDEX_H

#include "core/index/connection_index.h"

namespace logicsim {

/**
 * @brief: Index used for generating segment trees from line trees.
 */
struct GenerationIndex {
    explicit GenerationIndex() = default;
    explicit GenerationIndex(const Layout& layout);

    LogicItemInputIndex inputs {};
    LogicItemOutputIndex outputs {};

    [[nodiscard]] auto operator==(const GenerationIndex&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<GenerationIndex>);

}  // namespace logicsim

#endif
