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
        "  logicitem_keys_ = {}\n"
        "  logicitem_ids_ = {}\n"
        "  \n"
        "  decoration_keys_ = {}\n"
        "  decoration_ids_ = {}\n"
        ")",
        logicitem_keys_, logicitem_ids_, decoration_keys_, decoration_ids_);
}

auto KeyIndex::allocated_size() const -> std::size_t {
    assert(class_invariant_holds());

    return get_allocated_size(logicitem_keys_) +   //
           get_allocated_size(logicitem_ids_) +    //
                                                   //
           get_allocated_size(decoration_keys_) +  //
           get_allocated_size(decoration_ids_);
}

namespace {

template <typename K, typename V>
auto _get_value(const KeyIndex::map_type<K, V> &map, K key) -> V {
    if (const auto it = map.find(key); it != map.end()) {
        return it->second;
    }
    throw std::runtime_error("key does not exist");
}

template <typename Key, typename Id>
auto _set_new_key(KeyIndex::map_type<Key, Id>& map_ids,
                  KeyIndex::map_type<Id, Key>& map_keys, Id id, Key key) -> void {
    const auto key_it = map_keys.find(id);
    Expects(key_it != map_keys.end());

    const auto id_it = map_ids.find(key_it->second);
    Expects(id_it != map_ids.end());
    Expects(id_it->second == id);

    key_it->second = key;
    map_ids.erase(id_it);
    Expects(map_ids.emplace(key, id).second);
}

template <typename Id, typename Key>
auto _set_new_id(KeyIndex::map_type<Key, Id>& map_ids,
                 KeyIndex::map_type<Id, Key>& map_keys, Id old_id, Id new_id) -> void {
    const auto key_it = map_keys.find(old_id);
    Expects(key_it != map_keys.end());

    const auto id_it = map_ids.find(key_it->second);
    Expects(id_it != map_ids.end());
    Expects(id_it->second == old_id);

    id_it->second = new_id;
    map_keys.erase(key_it);
    Expects(map_keys.emplace(new_id, id_it->first).second);
}

template <typename Id, typename Key>
auto _delete_id(KeyIndex::map_type<Key, Id>& map_ids,
                KeyIndex::map_type<Id, Key>& map_keys, Id id) -> void {
    const auto key_it = map_keys.find(id);
    Expects(key_it != map_keys.end());

    const auto id_it = map_ids.find(key_it->second);
    Expects(id_it != map_ids.end());
    Expects(id_it->second == id);

    map_keys.erase(key_it);
    map_ids.erase(id_it);
}

}  // namespace

auto KeyIndex::get(logicitem_id_t logicitem_id) const -> logicitem_key_t {
    assert(class_invariant_holds());

    const auto res = _get_value(logicitem_keys_, logicitem_id);
    Ensures(res);

    return res;
}

auto KeyIndex::get(logicitem_key_t logicitem_key) const -> logicitem_id_t {
    assert(class_invariant_holds());

    const auto res = _get_value(logicitem_ids_, logicitem_key);
    Ensures(res);

    return res;
}

auto KeyIndex::set(logicitem_id_t logicitem_id, logicitem_key_t logicitem_key) -> void {
    assert(class_invariant_holds());

    _set_new_key(logicitem_ids_, logicitem_keys_, logicitem_id, logicitem_key);

    if (logicitem_key >= next_logicitem_key_) {
        next_logicitem_key_ = logicitem_key;
        ++next_logicitem_key_;
    }

    assert(class_invariant_holds());
}

auto KeyIndex::get(decoration_id_t decoration_id) const -> decoration_key_t {
    assert(class_invariant_holds());

    const auto res = _get_value(decoration_keys_, decoration_id);
    Ensures(res);

    return res;
}

auto KeyIndex::get(decoration_key_t decoration_key) const -> decoration_id_t {
    assert(class_invariant_holds());

    const auto res = _get_value(decoration_ids_, decoration_key);
    Ensures(res);

    return res;
}

auto KeyIndex::set(decoration_id_t decoration_id,
                   decoration_key_t decoration_key) -> void {
    assert(class_invariant_holds());

    _set_new_key(decoration_ids_, decoration_keys_, decoration_id, decoration_key);

    if (decoration_key >= next_decoration_key_) {
        next_decoration_key_ = decoration_key;
        ++next_decoration_key_;
    }

    assert(class_invariant_holds());
}

auto KeyIndex::handle(const info_message::LogicItemCreated& message) -> void {
    Expects(logicitem_ids_.emplace(next_logicitem_key_, message.logicitem_id).second);
    Expects(logicitem_keys_.emplace(message.logicitem_id, next_logicitem_key_).second);

    ++next_logicitem_key_;
}

auto KeyIndex::handle(const info_message::LogicItemIdUpdated& message) -> void {
    _set_new_id(logicitem_ids_, logicitem_keys_, message.old_logicitem_id,
                message.new_logicitem_id);
}

auto KeyIndex::handle(const info_message::LogicItemDeleted& message) -> void {
    _delete_id(logicitem_ids_, logicitem_keys_, message.logicitem_id);
}

auto KeyIndex::handle(const info_message::DecorationCreated& message) -> void {
    Expects(decoration_ids_.emplace(next_decoration_key_, message.decoration_id).second);
    Expects(decoration_keys_.emplace(message.decoration_id, next_decoration_key_).second);

    ++next_decoration_key_;
}

auto KeyIndex::handle(const info_message::DecorationIdUpdated& message) -> void {
    _set_new_id(decoration_ids_, decoration_keys_, message.old_decoration_id,
                message.new_decoration_id);
}

auto KeyIndex::handle(const info_message::DecorationDeleted& message) -> void {
    _delete_id(decoration_ids_, decoration_keys_, message.decoration_id);
}

auto KeyIndex::submit(const InfoMessage& message) -> void {
    assert(class_invariant_holds());

    using namespace info_message;

    // logic items
    if (auto pointer = std::get_if<LogicItemCreated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (auto pointer = std::get_if<LogicItemDeleted>(&message)) {
        handle(*pointer);
        return;
    }

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

[[nodiscard]] auto _has_all_logicitem_ids(
    const Layout& layout,
    const KeyIndex::map_type<logicitem_id_t, logicitem_key_t>& logicitem_keys) -> bool {
    if (logicitem_keys.empty()) {
        return layout.logicitems().empty();
    }

    const auto max_id = std::ranges::max(logicitem_keys | std::ranges::views::keys);
    return size_t {max_id} + 1 == layout.logicitems().size();
}

[[nodiscard]] auto _has_all_decoration_ids(
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

    return _has_all_logicitem_ids(layout, logicitem_keys_) &&
           _has_all_decoration_ids(layout, decoration_keys_);
}

namespace {

template <typename K, typename V>
auto _entries_valid(const KeyIndex::map_type<K, V>& map) -> bool {
    const auto entry_valid = [&](const std::pair<K, V>& entry) -> bool {
        return bool {entry.first} && bool {entry.second};
    };
    return std::ranges::all_of(map, entry_valid);
}

template <typename Id, typename Key>
auto _entries_consistent(const KeyIndex::map_type<Key, Id>& map_ids,
                         const KeyIndex::map_type<Id, Key>& map_keys) -> bool {
    const auto entry_consistent = [&](const std::pair<Id, Key>& entry) -> bool {
        const auto it = map_ids.find(entry.second);
        Expects(it != map_ids.end());
        Expects(it->first && it->second);

        return it->second == entry.first;
    };
    return std::ranges::all_of(map_keys, entry_consistent);
}

template <typename Id, typename Key>
auto _get_max_key(const KeyIndex::map_type<Key, Id>& map_ids) -> std::optional<Key> {
    if (map_ids.empty()) {
        return std::nullopt;
    }
    return std::ranges::max(map_ids | std::ranges::views::keys);
}

}  // namespace

auto KeyIndex::class_invariant_holds() const -> bool {
    // contain valid data
    Expects(_entries_valid(logicitem_ids_));
    Expects(_entries_valid(logicitem_keys_));
    Expects(_entries_valid(decoration_ids_));
    Expects(_entries_valid(decoration_keys_));

    // entries consistent
    Expects(_entries_consistent(logicitem_ids_, logicitem_keys_));
    Expects(_entries_consistent(decoration_ids_, decoration_keys_));

    // next key
    Expects(logicitem_ids_.empty() || next_logicitem_key_ > _get_max_key(logicitem_ids_));
    Expects(decoration_keys_.empty() ||
            next_decoration_key_ > _get_max_key(decoration_ids_));

    return true;
}

}  // namespace logicsim
