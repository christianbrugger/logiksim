#include "logicitem_store.h"
#include "logicitem_store.h"
#include "component/layout/logicitem_store.h"

#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/folly_small_vector.h"
#include "allocated_size/std_vector.h"
#include "layout_info.h"
#include "validate_definition.h"
#include "vocabulary/layout_calculation_data.h"

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include <stdexcept>

namespace logicsim {

namespace layout {

namespace {

/**
 * @brief: The value of the bounding rect, when it is not computed
 *
 * Note if an item is at this position with a zero bounding rect, we re-compute it
 * every frame. However this is very rare and even if it happens not a problem.
 */
constexpr inline auto invalid_bounding_rect =
    rect_t {point_t {-10'000, -10'000}, point_t {-10'000, -10'000}};

}  // namespace

auto LogicItemStore::size() const -> std::size_t {
    return logicitem_types_.size();
}

auto LogicItemStore::empty() const -> bool {
    return logicitem_types_.empty();
}

auto LogicItemStore::allocated_size() const -> std::size_t {
    return get_allocated_size(logicitem_types_) +  //
           get_allocated_size(input_counts_) +     //
           get_allocated_size(output_counts_) +    //
           get_allocated_size(orientations_) +     //

           get_allocated_size(sub_circuit_ids_) +   //
           get_allocated_size(input_inverters_) +   //
           get_allocated_size(output_inverters_) +  //

           get_allocated_size(positions_) +       //
           get_allocated_size(display_states_) +  //
           get_allocated_size(bounding_rects_) +  //

           get_allocated_size(map_clock_generator_);
}

auto LogicItemStore::add_logicitem(const ElementDefinition &definition, point_t position,
                                   display_state_t display_state) -> logicitem_id_t {
    if (!is_valid(definition)) [[unlikely]] {
        throw std::runtime_error("Invalid element definition.");
    }
    if (size() >= std::size_t {logicitem_id_t::max()} - std::size_t {1}) [[unlikely]] {
        throw std::runtime_error("Reached maximum number of logic items.");
    }

    const auto logicitem_id =
        logicitem_id_t {gsl::narrow_cast<logicitem_id_t::value_type>(size())};

    // extend vectors
    logicitem_types_.push_back(definition.logicitem_type);
    input_counts_.push_back(definition.input_count);
    output_counts_.push_back(definition.output_count);
    orientations_.push_back(definition.orientation);
    sub_circuit_ids_.push_back(definition.sub_circuit_id);

    if (definition.input_inverters.empty()) {
        input_inverters_.emplace_back(definition.input_count.count(), false);
    } else {
        if (definition.input_inverters.size() != std::size_t {definition.input_count})
            [[unlikely]] {
            throw std::runtime_error(
                "number of input inverters need to match input count");
        }
        input_inverters_.emplace_back(definition.input_inverters);
    }

    if (definition.output_inverters.empty()) {
        output_inverters_.emplace_back(definition.output_count.count(), false);
    } else {
        if (definition.output_inverters.size() != std::size_t {definition.output_count})
            [[unlikely]] {
            throw std::runtime_error(
                "number of output inverters need to match output count");
        }
        output_inverters_.emplace_back(definition.output_inverters);
    }

    positions_.push_back(position);
    display_states_.push_back(display_state);
    bounding_rects_.push_back(invalid_bounding_rect);

    // attributes
    const auto add_map_entry = [&](auto &map, const auto &optional) {
        if (optional && !map.emplace(logicitem_id, *optional).second) {
            throw std::runtime_error("logicitem id already exists in map");
        }
    };

    add_map_entry(map_clock_generator_, definition.attrs_clock_generator);

    return logicitem_id;
}

auto LogicItemStore::swap_and_delete(logicitem_id_t logicitem_id) -> logicitem_id_t {
    const auto last_id = last_logicitem_id();

    swap(logicitem_id, last_id);
    delete_last();

    return last_id;
}

auto LogicItemStore::swap(logicitem_id_t logicitem_id_1, logicitem_id_t logicitem_id_2)
    -> void {
    if (logicitem_id_1 == logicitem_id_2) {
        return;
    }

    const auto swap_ids = [logicitem_id_1, logicitem_id_2](auto &container) {
        using std::swap;
        swap(container.at(logicitem_id_1.value), container.at(logicitem_id_2.value));
    };

    // TODO put in algorithm
    const auto swap_map_ids = [logicitem_id_1, logicitem_id_2](auto &map) {
        auto it1 = map.find(logicitem_id_1);
        auto it2 = map.find(logicitem_id_2);

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
            map.emplace(logicitem_id_2, std::move(tmp));
            return;
        }

        if (it1 == map.end() && it2 != map.end()) {
            auto tmp = std::move(it2->second);
            map.erase(it2);
            map.emplace(logicitem_id_1, std::move(tmp));
            return;
        }

        throw std::runtime_error("unknown case in swap_map_ids");
    };

    swap_ids(logicitem_types_);
    swap_ids(input_counts_);
    swap_ids(output_counts_);
    swap_ids(orientations_);

    swap_ids(sub_circuit_ids_);
    swap_ids(input_inverters_);
    swap_ids(output_inverters_);

    swap_ids(positions_);
    swap_ids(display_states_);
    swap_ids(bounding_rects_);

    swap_map_ids(map_clock_generator_);
}

auto LogicItemStore::normalize() -> void {
    // clear caches
    std::ranges::fill(bounding_rects_, invalid_bounding_rect);

    // sort
    const auto vectors = ranges::zip_view(  //
        logicitem_types_,                   //
        input_counts_,                      //
        output_counts_,                     //
        orientations_,                      //
                                            //
        sub_circuit_ids_,                   //
        input_inverters_,                   //
        output_inverters_,                  //
                                            //
        positions_,                         //
        display_states_                     //
    );

    // TODO !!! clock attributes !!!

    ranges::sort(vectors);
}

auto LogicItemStore::operator==(const LogicItemStore &other) const -> bool {
    // caches are not part of our value
    return logicitem_types_ == other.logicitem_types_ &&
           input_counts_ == other.input_counts_ &&
           output_counts_ == other.output_counts_ &&
           orientations_ == other.orientations_ &&

           sub_circuit_ids_ == other.sub_circuit_ids_ &&
           input_inverters_ == other.input_inverters_ &&
           output_inverters_ == other.output_inverters_ &&

           positions_ == other.positions_ &&  //
           display_states_ == other.display_states_ &&

           map_clock_generator_ == other.map_clock_generator_;
}

auto LogicItemStore::type(logicitem_id_t logicitem_id) const -> LogicItemType {
    return logicitem_types_.at(logicitem_id.value);
}

auto LogicItemStore::input_count(logicitem_id_t logicitem_id) const
    -> connection_count_t {
    return input_counts_.at(logicitem_id.value);
}

auto LogicItemStore::output_count(logicitem_id_t logicitem_id) const
    -> connection_count_t {
    return output_counts_.at(logicitem_id.value);
}

auto LogicItemStore::orientation(logicitem_id_t logicitem_id) const -> orientation_t {
    return orientations_.at(logicitem_id.value);
}

auto LogicItemStore::sub_circuit_id(logicitem_id_t logicitem_id) const -> circuit_id_t {
    return sub_circuit_ids_.at(logicitem_id.value);
}

auto LogicItemStore::input_inverters(logicitem_id_t logicitem_id) const
    -> logic_small_vector_t {
    return input_inverters_.at(logicitem_id.value);
}

auto LogicItemStore::output_inverters(logicitem_id_t logicitem_id) const
    -> logic_small_vector_t {
    return output_inverters_.at(logicitem_id.value);
}

auto LogicItemStore::position(logicitem_id_t logicitem_id) const -> point_t {
    return positions_.at(logicitem_id.value);
}

auto LogicItemStore::display_state(logicitem_id_t logicitem_id) const -> display_state_t {
    return display_states_.at(logicitem_id.value);
}

auto LogicItemStore::bounding_rect(logicitem_id_t logicitem_id) const -> rect_t {
    auto &rect = bounding_rects_.at(logicitem_id.value);

    if (rect == invalid_bounding_rect) {
        // update bounding rect
        const auto data = to_layout_calculation_data(*this, logicitem_id);
        rect = element_bounding_rect(data);
    }

    return rect;
}

auto LogicItemStore::attrs_clock_generator(logicitem_id_t logicitem_id) const
    -> const attributes_clock_generator_t & {
    const auto it = map_clock_generator_.find(logicitem_id);

    if (it == map_clock_generator_.end()) {
        throw std::runtime_error("could not find attribute");
    }

    return it->second;
}

auto LogicItemStore::input_inverted(logicitem_id_t logicitem_id,
                                    connection_id_t input_id) const -> bool {
    return input_inverters(logicitem_id).at(input_id.value);
}

auto LogicItemStore::output_inverted(logicitem_id_t logicitem_id,
                                     connection_id_t output_id) const -> bool {
    return output_inverters(logicitem_id).at(output_id.value);
}

auto LogicItemStore::set_position(logicitem_id_t logicitem_id, point_t position) -> void {
    // reset caches
    positions_.at(logicitem_id.value) = position;

    bounding_rects_.at(logicitem_id.value) = invalid_bounding_rect;
}

auto LogicItemStore::set_display_state(logicitem_id_t logicitem_id,
                                       display_state_t display_state) -> void {
    display_states_.at(logicitem_id.value) = display_state;
}

auto LogicItemStore::set_attributes(logicitem_id_t logicitem_id,
                                    attributes_clock_generator_t attrs) -> void {
    const auto it = map_clock_generator_.find(logicitem_id);

    if (it == map_clock_generator_.end()) [[unlikely]] {
        throw std::runtime_error("could not find attribute");
    }
    if (!is_valid(attrs)) [[unlikely]] {
        throw std::runtime_error("attributes not valid");
    }

    it->second = std::move(attrs);
}

auto LogicItemStore::delete_last() -> void {
    if (empty()) {
        throw std::runtime_error("Cannot delete last logicitem of empty layout.");
    }

    const auto last_id = last_logicitem_id();

    logicitem_types_.pop_back();
    input_counts_.pop_back();
    output_counts_.pop_back();
    orientations_.pop_back();

    sub_circuit_ids_.pop_back();
    input_inverters_.pop_back();
    output_inverters_.pop_back();

    positions_.pop_back();
    display_states_.pop_back();
    bounding_rects_.pop_back();

    map_clock_generator_.erase(last_id);
}

auto LogicItemStore::last_logicitem_id() -> logicitem_id_t {
    return logicitem_id_t {
        gsl::narrow_cast<logicitem_id_t::value_type>(size() - std::size_t {1})};
}

//
// Free Functions
//

auto to_layout_calculation_data(const LogicItemStore &store, logicitem_id_t logicitem_id)
    -> layout_calculation_data_t {
    return layout_calculation_data_t {
        .internal_state_count = 0,  // TODO get count when implemented
        .position = store.position(logicitem_id),
        .input_count = store.input_count(logicitem_id),
        .output_count = store.output_count(logicitem_id),
        .orientation = store.orientation(logicitem_id),
        .logicitem_type = store.type(logicitem_id),
    };
}

auto to_logicitem_definition(const LogicItemStore &store, logicitem_id_t logicitem_id)
    -> ElementDefinition {
    return ElementDefinition {
        .logicitem_type = store.type(logicitem_id),
        .input_count = store.input_count(logicitem_id),
        .output_count = store.output_count(logicitem_id),
        .orientation = store.orientation(logicitem_id),

        .sub_circuit_id = store.sub_circuit_id(logicitem_id),
        .input_inverters = store.input_inverters(logicitem_id),
        .output_inverters = store.output_inverters(logicitem_id),

        .attrs_clock_generator =
            store.type(logicitem_id) == LogicItemType::clock_generator
                ? std::make_optional(store.attrs_clock_generator(logicitem_id))
                : std::nullopt,
    };
}

}  // namespace layout

}  // namespace logicsim
