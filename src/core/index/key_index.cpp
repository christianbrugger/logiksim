#include "core/index/key_index.h"

#include "core/algorithm/to_vector.h"
#include "core/allocated_size/ankerl_unordered_dense.h"
#include "core/layout_message.h"
#include "core/layout_message_generation.h"
#include "core/logging.h"

#include <fmt/core.h>

namespace logicsim {

KeyIndex::KeyIndex(const Layout& layout) {
    generate_created_layout_messages(*this, layout);

    assert(class_invariant_holds());
    assert(has_all_ids_inserted(layout));
}

auto KeyIndex::format() const -> std::string {
    assert(class_invariant_holds());

    return fmt::format(
        "KeyIndex(\n"
        "  decoration_keys_ = {}\n"
        "  decoration_ids_ = {}\n"
        ")",
        decoration_keys_, decoration_ids_);
}

auto KeyIndex::allocated_size() const -> std::size_t {
    assert(class_invariant_holds());

    return get_allocated_size(decoration_keys_) +  //
           get_allocated_size(decoration_ids_);
}

auto KeyIndex::handle(const info_message::DecorationCreated& message) -> void {
    Expects(decoration_ids_.emplace(next_decoration_key_, message.decoration_id).second);
    Expects(decoration_keys_.emplace(message.decoration_id, next_decoration_key_).second);

    ++next_decoration_key_;
}

auto KeyIndex::handle(const info_message::DecorationIdUpdated& message) -> void {
    const auto key_it = decoration_keys_.find(message.old_decoration_id);
    Expects(key_it != decoration_keys_.end());

    const auto id_it = decoration_ids_.find(key_it->second);
    Expects(id_it != decoration_ids_.end());
    Expects(id_it->second == message.old_decoration_id);

    id_it->second = message.new_decoration_id;
    decoration_keys_.erase(key_it);
    Expects(decoration_keys_.emplace(message.new_decoration_id, id_it->first).second);
}

auto KeyIndex::handle(const info_message::DecorationDeleted& message) -> void {
    const auto key_it = decoration_keys_.find(message.decoration_id);
    Expects(key_it != decoration_keys_.end());

    const auto id_it = decoration_ids_.find(key_it->second);
    Expects(id_it != decoration_ids_.end());
    Expects(id_it->second == message.decoration_id);

    decoration_keys_.erase(key_it);
    decoration_ids_.erase(id_it);
}

auto KeyIndex::submit(const InfoMessage& message) -> void {
    assert(class_invariant_holds());

    using namespace info_message;

    // decorations
    if (auto pointer = std::get_if<DecorationCreated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<DecorationIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<DecorationDeleted>(&message)) {
        handle(*pointer);
        return;
    }

    assert(class_invariant_holds());
}

namespace {

[[nodiscard]] auto has_all_decoration_ids(
    const Layout& layout,
    const KeyIndex::map_type<decoration_id_t, decoration_key_t>& decoration_keys)
    -> bool {
    if (decoration_keys.empty()) {
        return layout.decorations().empty();
    }

    const auto max_id = std::ranges::max(decoration_keys | std::ranges::views::keys);
    return size_t {max_id} + 1 == layout.decorations().size();
}

}  // namespace

auto KeyIndex::has_all_ids_inserted(const Layout& layout) const -> bool {
    assert(class_invariant_holds());

    return has_all_decoration_ids(layout, decoration_keys_);
}

auto KeyIndex::class_invariant_holds() const -> bool {
    // all decorations consistent
    const auto decoration_entry_consistent =
        [&](const std::pair<decoration_id_t, decoration_key_t>& entry) -> bool {
        Expects(entry.first && entry.second);

        const auto it = decoration_ids_.find(entry.second);
        Expects(it != decoration_ids_.end());
        Expects(it->first && it->second);

        return it->second == entry.first;
    };
    Expects(std::ranges::all_of(decoration_keys_, decoration_entry_consistent));

    // next decoration key
    if (!decoration_keys_.empty()) {
        Expects(next_decoration_key_ >
                std::ranges::max(decoration_ids_ | std::ranges::views::keys));
    }

    return true;
}

}  // namespace logicsim
