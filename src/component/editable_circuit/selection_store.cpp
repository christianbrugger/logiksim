#include "selection_store.h"

#include "algorithm/fmt_join.h"
#include "algorithm/uniform_int_distribution.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "component/editable_circuit/selection_store.h"
#include "format/container.h"
#include "format/std_type.h"

#include <fmt/core.h>

#include <limits>
#include <random>

namespace logicsim {

namespace editable_circuit {

namespace {

/**
 * @brief: Return random start value for selection ids.
 *
 * By setting a random start value for selection ids, it is very unlikely
 * that two selection ids for two different editable-circuit with different
 * selection stores are the same.
 */
auto get_random_start_id() -> selection_id_t {
    constexpr auto max_value = std::numeric_limits<int32_t>::max();

    auto rd = std::random_device {};
    auto dist = uint_distribution(int64_t {0}, int64_t {max_value});

    return selection_id_t {dist(rd)};
}

}  // namespace

SelectionStore::SelectionStore() : next_selection_key_ {get_random_start_id()} {}

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

auto SelectionStore::contains(selection_id_t selection_id) const -> bool {
    return bool {selection_id} && selections_.contains(selection_id);
}

auto SelectionStore::create() -> selection_id_t {
    const auto selection_id = next_selection_key_++;

    const auto&& [it, inserted] = selections_.emplace(selection_id, Selection {});
    Ensures(inserted);

    return selection_id;
}

auto SelectionStore::destroy(selection_id_t selection_id) -> void {
    if (const auto it = selections_.find(selection_id); it != selections_.end()) {
        selections_.erase(it);
        return;
    }
    throw std::runtime_error("Selection not found in selection store remove.");
}

auto SelectionStore::submit(const InfoMessage& message) -> void {
    for (auto& item : selections_) {
        item.second.submit(message);
    }
}

}  // namespace editable_circuit

}  // namespace logicsim
