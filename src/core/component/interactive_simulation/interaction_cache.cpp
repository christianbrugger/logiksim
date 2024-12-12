#include "core/component/interactive_simulation/interaction_cache.h"

#include "core/allocated_size/ankerl_unordered_dense.h"
#include "core/format/std_type.h"
#include "core/layout.h"
#include "core/schematic_generation.h"

namespace logicsim {

namespace interactive_simulation {

auto interaction_data_t::format() const -> std::string {
    return fmt::format("{}", element_id);
}

InteractionCache::InteractionCache(const Layout& layout) {
    for (auto logicitem_id : logicitem_ids(layout)) {
        if (layout.logicitems().type(logicitem_id) == LogicItemType::button) {
            auto& data = map_[layout.logicitems().position(logicitem_id)];

            if (data.element_id != null_element) [[unlikely]] {
                throw std::runtime_error("map entry is not empty");
            }

            data.element_id = to_element_id(layout, logicitem_id);
        }
    }
}

auto InteractionCache::format() const -> std::string {
    return fmt::format("<InteractionCache: {}>", map_);
}

auto InteractionCache::allocated_size() const -> std::size_t {
    return get_allocated_size(map_);
}

auto InteractionCache::find(point_t position) const -> std::optional<element_id_t> {
    if (const auto it = map_.find(position); it != map_.end()) {
        return it->second.element_id;
    }
    return std::nullopt;
}

}  // namespace interactive_simulation

}  // namespace logicsim
