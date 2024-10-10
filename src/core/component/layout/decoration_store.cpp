#include "component/layout/decoration_store.h"

#include "algorithm/range_extended.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/std_vector.h"
#include "layout_info.h"
#include "validate_definition_decoration.h"
#include "vocabulary/decoration_layout_data.h"

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include <stdexcept>

namespace logicsim {

namespace layout {

auto DecorationStore::size() const -> std::size_t {
    return decoration_types_.size();
}

auto DecorationStore::empty() const -> bool {
    return decoration_types_.empty();
}

auto DecorationStore::allocated_size() const -> std::size_t {
    return get_allocated_size(decoration_types_) +  //
           get_allocated_size(sizes_) +             //

           get_allocated_size(positions_) +       //
           get_allocated_size(display_states_) +  //
           get_allocated_size(bounding_rects_) +  //

           get_allocated_size(map_text_element_);
}

auto DecorationStore::add(const DecorationDefinition &definition, point_t position,
                          display_state_t display_state) -> decoration_id_t {
    if (!is_valid(definition)) [[unlikely]] {
        throw std::runtime_error("Invalid decoration definition.");
    }
    if (size() >= std::size_t {decoration_id_t::max()} - std::size_t {1}) [[unlikely]] {
        throw std::runtime_error("Reached maximum number of decorations.");
    }
    // throws if its not representable
    const auto bounding_rect =
        element_bounding_rect(to_decoration_layout_data(definition, position));

    const auto decoration_id =
        decoration_id_t {gsl::narrow_cast<decoration_id_t::value_type>(size())};

    // extend vectors
    decoration_types_.push_back(definition.decoration_type);
    sizes_.push_back(definition.size);

    positions_.push_back(position);
    display_states_.push_back(display_state);
    bounding_rects_.push_back(bounding_rect);

    // attributes
    const auto add_map_entry = [&](auto &map, const auto &optional) {
        if (optional && !map.emplace(decoration_id, *optional).second) {
            std::terminate();  // value already esists
        }
    };

    add_map_entry(map_text_element_, definition.attrs_text_element);

    return decoration_id;
}

auto DecorationStore::swap_and_delete(decoration_id_t decoration_id) -> decoration_id_t {
    const auto last_id = last_decoration_id();

    swap_items(decoration_id, last_id);
    delete_last();

    return last_id;
}

auto DecorationStore::swap_items(decoration_id_t decoration_id_1,
                                 decoration_id_t decoration_id_2) -> void {
    if (decoration_id_1 == decoration_id_2) {
        return;
    }

    const auto swap_ids = [decoration_id_1, decoration_id_2](auto &container) {
        using std::swap;
        swap(container.at(decoration_id_1.value), container.at(decoration_id_2.value));
    };

    // TODO put in algorithm
    const auto swap_map_ids = [decoration_id_1, decoration_id_2](auto &map) {
        auto it1 = map.find(decoration_id_1);
        auto it2 = map.find(decoration_id_2);

        if (it1 == map.end() && it2 == map.end()) {
            return;
        }

        if (it1 != map.end() && it2 != map.end()) {
            using std::swap;
            swap(it1->second, it2->second);
            return;
        }

        if (it1 != map.end() && it2 == map.end()) {
            auto tmp = std::move(it1->second);
            map.erase(it1);
            map.emplace(decoration_id_2, std::move(tmp));
            return;
        }

        if (it1 == map.end() && it2 != map.end()) {
            auto tmp = std::move(it2->second);
            map.erase(it2);
            map.emplace(decoration_id_1, std::move(tmp));
            return;
        }

        throw std::runtime_error("unknown case in swap_map_ids");
    };

    swap_ids(decoration_types_);
    swap_ids(sizes_);

    swap_ids(positions_);
    swap_ids(display_states_);
    swap_ids(bounding_rects_);

    swap_map_ids(map_text_element_);
}

namespace {

// TODO abstract put in algorithm
auto move_to_vector(layout::decoration_attr_t<attributes_text_element_t> &map,
                    std::size_t size) {
    auto result =
        std::vector<std::optional<attributes_text_element_t>>(size, std::nullopt);

    for (auto &&[decoration_id, attr] : map) {
        result.at(decoration_id.value) = std::move(attr);
    }

    map.clear();
    return result;
}

// TODO abstract put in algorithm
auto move_from_vector(std::vector<std::optional<attributes_text_element_t>> &&vector) {
    auto map = layout::decoration_attr_t<attributes_text_element_t> {};

    for (decoration_id_t decoration_id : range_extended<decoration_id_t>(vector.size())) {
        auto &entry = vector[decoration_id.value];

        if (entry.has_value()) {
            map[decoration_id] = std::move(entry.value());
        }
    }

    return map;
}

}  // namespace

auto DecorationStore::normalize() -> void {
    auto vector_text_element = move_to_vector(map_text_element_, size());

    // sort
    const auto vectors = ranges::zip_view(  //
        decoration_types_,                  //
        sizes_,                             //
                                            //
        positions_,                         //
        display_states_,                    //
        bounding_rects_,                    //
                                            //
        vector_text_element                 //
    );
    ranges::sort(vectors);

    map_text_element_ = move_from_vector(std::move(vector_text_element));
}

auto DecorationStore::type(decoration_id_t decoration_id) const -> DecorationType {
    return decoration_types_.at(decoration_id.value);
}

auto DecorationStore::size(decoration_id_t decoration_id) const -> size_2d_t {
    return sizes_.at(decoration_id.value);
}

auto DecorationStore::position(decoration_id_t decoration_id) const -> point_t {
    return positions_.at(decoration_id.value);
}

auto DecorationStore::display_state(decoration_id_t decoration_id) const
    -> display_state_t {
    return display_states_.at(decoration_id.value);
}

auto DecorationStore::bounding_rect(decoration_id_t decoration_id) const -> rect_t {
    return bounding_rects_.at(decoration_id.value);
}

auto DecorationStore::attrs_text_element(decoration_id_t decoration_id) const
    -> const attributes_text_element_t & {
    const auto it = map_text_element_.find(decoration_id);

    if (it == map_text_element_.end()) {
        throw std::runtime_error("could not find attribute");
    }

    return it->second;
}

auto DecorationStore::set_position(decoration_id_t decoration_id,
                                   point_t position) -> void {
    // throws if it is not representable
    const auto bounding_rect =
        element_bounding_rect(to_decoration_layout_data(*this, decoration_id, position));

    // set new position
    positions_.at(decoration_id.value) = position;
    bounding_rects_.at(decoration_id.value) = bounding_rect;
}

auto DecorationStore::set_display_state(decoration_id_t decoration_id,
                                        display_state_t display_state) -> void {
    display_states_.at(decoration_id.value) = display_state;
}

auto DecorationStore::set_attributes(decoration_id_t decoration_id,
                                     attributes_text_element_t attrs) -> void {
    const auto it = map_text_element_.find(decoration_id);

    if (it == map_text_element_.end()) [[unlikely]] {
        throw std::runtime_error("could not find attribute");
    }
    if (!is_valid(attrs)) [[unlikely]] {
        throw std::runtime_error("attributes not valid");
    }

    it->second = std::move(attrs);
}

auto DecorationStore::delete_last() -> void {
    if (empty()) {
        throw std::runtime_error("Cannot delete last decoration of empty layout.");
    }

    const auto last_id = last_decoration_id();

    decoration_types_.pop_back();
    sizes_.pop_back();

    positions_.pop_back();
    display_states_.pop_back();
    bounding_rects_.pop_back();

    map_text_element_.erase(last_id);
}

auto DecorationStore::last_decoration_id() const -> decoration_id_t {
    return decoration_id_t {
        gsl::narrow_cast<decoration_id_t::value_type>(size() - std::size_t {1})};
}

//
// Free Functions
//

auto to_decoration_layout_data(const DecorationStore &store,
                               decoration_id_t decoration_id)
    -> decoration_layout_data_t {
    return to_decoration_layout_data(store, decoration_id, store.position(decoration_id));
}

auto to_decoration_layout_data(const DecorationStore &store,
                               decoration_id_t decoration_id,
                               point_t position) -> decoration_layout_data_t {
    return decoration_layout_data_t {
        .position = position,
        .size = store.size(decoration_id),
        .decoration_type = store.type(decoration_id),
    };
}

auto to_decoration_definition(const DecorationStore &store,
                              decoration_id_t decoration_id) -> DecorationDefinition {
    const auto type = store.type(decoration_id);

    return DecorationDefinition {
        .size = store.size(decoration_id),
        .decoration_type = type,

        .attrs_text_element =
            type == DecorationType::text_element
                ? std::make_optional(store.attrs_text_element(decoration_id))
                : std::nullopt,
    };
}

}  // namespace layout

}  // namespace logicsim
