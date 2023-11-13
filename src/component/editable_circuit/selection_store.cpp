#include "component/editable_circuit/selection_store.h"

#include "algorithm/fmt_join.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "format/container.h"
#include "format/std_type.h"

#include <fmt/core.h>

namespace logicsim {

namespace editable_circuit {

auto SelectionStore::format() const -> std::string {
    const auto item_str = fmt_join(",\n", selections_.values());
    return fmt::format("SelectionStore({})", item_str);
}

auto SelectionStore::allocated_size() const -> std::size_t {
    return get_allocated_size(selections_);
}

auto SelectionStore::clear() -> void {
    selections_.clear();
}

auto SelectionStore::empty() const -> bool {
    return selections_.empty();
}

auto SelectionStore::size() const -> std::size_t {
    return selections_.size();
}

auto SelectionStore::begin() -> iterator {
    return selections_.begin();
}

auto SelectionStore::end() -> iterator {
    return selections_.end();
}

auto SelectionStore::begin() const -> const_iterator {
    return selections_.begin();
}

auto SelectionStore::end() const -> const_iterator {
    return selections_.end();
}

auto SelectionStore::at(selection_id_t selection_id) -> Selection& {
    if (const auto it = selections_.find(selection_id); it != selections_.end()) {
        return it->second;
    }

    throw std::runtime_error("Selection not found in selection store.");
}

auto SelectionStore::at(selection_id_t selection_id) const -> const Selection& {
    if (const auto it = selections_.find(selection_id); it != selections_.end()) {
        return it->second;
    }

    throw std::runtime_error("Selection not found in selection store.");
}

auto SelectionStore::create() -> selection_id_t {
    const auto selection_id = next_selection_key_++;

    const auto&& [it, inserted] = selections_.emplace(selection_id, Selection {});
    Ensures(inserted);

    return selection_id;
}

auto SelectionStore::remove(selection_id_t selection_id) -> void {
    if (const auto it = selections_.find(selection_id); it != selections_.end()) {
        selections_.erase(it);
    }
    throw std::runtime_error("Selection not found in selection store remove.");
}

}  // namespace editable_circuit

}  // namespace logicsim
