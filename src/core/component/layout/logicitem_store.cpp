#include "core/component/layout/logicitem_store.h"

#include "core/algorithm/range_extended.h"
#include "core/allocated_size/ankerl_unordered_dense.h"
#include "core/allocated_size/folly_small_vector.h"
#include "core/allocated_size/std_vector.h"
#include "core/component/layout/logicitem_store.h"
#include "core/layout_info.h"
#include "core/validate_definition_logicitem.h"
#include "core/vocabulary/layout_calculation_data.h"
#include "core/vocabulary/placed_logicitem.h"

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include <stdexcept>

namespace logicsim {

namespace layout {

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

auto LogicItemStore::add(LogicItemDefinition &&definition, point_t position,
                         display_state_t display_state) -> logicitem_id_t {
    if (!is_valid(definition)) [[unlikely]] {
        throw std::runtime_error("Invalid element definition.");
    }
    if (size() >= std::size_t {logicitem_id_t::max()} - std::size_t {1}) [[unlikely]] {
        throw std::runtime_error("Reached maximum number of logic items.");
    }
    if (!definition.input_inverters.empty() &&
        definition.input_inverters.size() != std::size_t {definition.input_count})
        [[unlikely]] {
        throw std::runtime_error("number of input inverters need to match input count");
    }
    if (!definition.output_inverters.empty() &&
        definition.output_inverters.size() != std::size_t {definition.output_count})
        [[unlikely]] {
        throw std::runtime_error("number of output inverters need to match output count");
    }
    // throws if its not representable
    const auto bounding_rect =
        element_bounding_rect(to_layout_calculation_data(definition, position));

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
        input_inverters_.emplace_back(std::move(definition.input_inverters));
    }
    if (definition.output_inverters.empty()) {
        output_inverters_.emplace_back(definition.output_count.count(), false);
    } else {
        output_inverters_.emplace_back(std::move(definition.output_inverters));
    }

    positions_.push_back(position);
    display_states_.push_back(display_state);
    bounding_rects_.push_back(bounding_rect);

    // attributes
    if (definition.logicitem_type == LogicItemType::clock_generator) {
        Expects(map_clock_generator_
                    .emplace(logicitem_id,
                             std::move(definition.attrs_clock_generator.value()))
                    .second);
    }

    return logicitem_id;
}

auto LogicItemStore::swap_and_delete(logicitem_id_t logicitem_id)
    -> std::pair<logicitem_id_t, PlacedLogicItem> {
    const auto last_id = last_logicitem_id();

    swap_items(logicitem_id, last_id);

    return {last_id, delete_last()};
}

auto LogicItemStore::swap_items(logicitem_id_t logicitem_id_1,
                                logicitem_id_t logicitem_id_2) -> void {
    if (logicitem_id_1 == logicitem_id_2) {
        return;
    }

    const auto swap_ids = [logicitem_id_1, logicitem_id_2](auto &container) {
        using std::swap;
        swap(container.at(size_t {logicitem_id_1}),
             container.at(size_t {logicitem_id_2}));
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

namespace {

auto move_to_vector(layout::attr_map_t<attributes_clock_generator_t> &map,
                    std::size_t size) {
    auto result =
        std::vector<std::optional<attributes_clock_generator_t>>(size, std::nullopt);

    for (auto &&[logicitem_id, attr] : map) {
        result.at(size_t {logicitem_id}) = std::move(attr);
    }

    map.clear();
    return result;
}

auto move_from_vector(std::vector<std::optional<attributes_clock_generator_t>> &&vector) {
    auto map = layout::attr_map_t<attributes_clock_generator_t> {};

    for (logicitem_id_t logicitem_id :
         range_extended<logicitem_id_t>(std::ssize(vector))) {
        auto &entry = vector.at(std::size_t {logicitem_id});

        if (entry.has_value()) {
            map[logicitem_id] = std::move(entry.value());
        }
    }

    return map;
}

}  // namespace

auto LogicItemStore::normalize() -> void {
    auto vector_clock_generator = move_to_vector(map_clock_generator_, size());

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
        display_states_,                    //
        bounding_rects_,                    //
        vector_clock_generator              //
    );
    ranges::sort(vectors);

    map_clock_generator_ = move_from_vector(std::move(vector_clock_generator));
}

auto LogicItemStore::type(logicitem_id_t logicitem_id) const -> LogicItemType {
    return logicitem_types_.at(size_t {logicitem_id});
}

auto LogicItemStore::input_count(logicitem_id_t logicitem_id) const
    -> connection_count_t {
    return input_counts_.at(size_t {logicitem_id});
}

auto LogicItemStore::output_count(logicitem_id_t logicitem_id) const
    -> connection_count_t {
    return output_counts_.at(size_t {logicitem_id});
}

auto LogicItemStore::orientation(logicitem_id_t logicitem_id) const -> orientation_t {
    return orientations_.at(size_t {logicitem_id});
}

auto LogicItemStore::sub_circuit_id(logicitem_id_t logicitem_id) const -> circuit_id_t {
    return sub_circuit_ids_.at(size_t {logicitem_id});
}

auto LogicItemStore::input_inverters(logicitem_id_t logicitem_id) const
    -> logic_small_vector_t {
    return input_inverters_.at(size_t {logicitem_id});
}

auto LogicItemStore::output_inverters(logicitem_id_t logicitem_id) const
    -> logic_small_vector_t {
    return output_inverters_.at(size_t {logicitem_id});
}

auto LogicItemStore::position(logicitem_id_t logicitem_id) const -> point_t {
    return positions_.at(size_t {logicitem_id});
}

auto LogicItemStore::display_state(logicitem_id_t logicitem_id) const -> display_state_t {
    return display_states_.at(size_t {logicitem_id});
}

auto LogicItemStore::bounding_rect(logicitem_id_t logicitem_id) const -> rect_t {
    return bounding_rects_.at(size_t {logicitem_id});
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
    return input_inverters(logicitem_id).at(size_t {input_id});
}

auto LogicItemStore::output_inverted(logicitem_id_t logicitem_id,
                                     connection_id_t output_id) const -> bool {
    return output_inverters(logicitem_id).at(size_t {output_id});
}

auto LogicItemStore::set_position(logicitem_id_t logicitem_id, point_t position) -> void {
    // throws if it is not representable
    const auto bounding_rect =
        element_bounding_rect(to_layout_calculation_data(*this, logicitem_id, position));

    // set new position
    positions_.at(size_t {logicitem_id}) = position;
    bounding_rects_.at(size_t {logicitem_id}) = bounding_rect;
}

auto LogicItemStore::set_display_state(logicitem_id_t logicitem_id,
                                       display_state_t display_state) -> void {
    display_states_.at(size_t {logicitem_id}) = display_state;
}

auto LogicItemStore::set_attributes(logicitem_id_t logicitem_id,
                                    attributes_clock_generator_t &&attrs)
    -> attributes_clock_generator_t {
    const auto it = map_clock_generator_.find(logicitem_id);

    if (it == map_clock_generator_.end()) [[unlikely]] {
        throw std::runtime_error("could not find attribute");
    }
    if (!is_valid(attrs)) [[unlikely]] {
        throw std::runtime_error("attributes not valid");
    }

    {
        using namespace std;
        swap(it->second, attrs);
    }
    return attrs;
}

auto LogicItemStore::set_input_inverter(logicitem_id_t logicitem_id,
                                        connection_id_t connection_id,
                                        bool value) -> void {
    input_inverters_.at(size_t {logicitem_id}).at(size_t {connection_id}) = value;
}

auto LogicItemStore::set_output_inverter(logicitem_id_t logicitem_id,
                                         connection_id_t connection_id,
                                         bool value) -> void {
    output_inverters_.at(size_t {logicitem_id}).at(size_t {connection_id}) = value;
}

auto LogicItemStore::delete_last() -> PlacedLogicItem {
    if (empty()) {
        throw std::runtime_error("Cannot delete last logicitem of empty layout.");
    }

    const auto last_id = last_logicitem_id();
    const auto it_clock_generator = map_clock_generator_.find(last_id);

    // move
    const auto result = PlacedLogicItem {
        .definition =
            LogicItemDefinition {
                .logicitem_type = logicitem_types_.back(),
                .input_count = input_counts_.back(),
                .output_count = output_counts_.back(),
                .orientation = orientations_.back(),
                //
                .sub_circuit_id = sub_circuit_ids_.back(),
                .input_inverters = std::move(input_inverters_.back()),
                .output_inverters = std::move(output_inverters_.back()),
                //
                .attrs_clock_generator =
                    it_clock_generator != map_clock_generator_.end()
                        ? std::make_optional(std::move(it_clock_generator->second))
                        : std::nullopt},
        .position = positions_.back(),
    };

    // pop
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

    if (it_clock_generator != map_clock_generator_.end()) {
        map_clock_generator_.erase(it_clock_generator);
    }

    return result;
}

auto LogicItemStore::last_logicitem_id() const -> logicitem_id_t {
    return logicitem_id_t {
        gsl::narrow_cast<logicitem_id_t::value_type>(size() - std::size_t {1})};
}

//
// Free Functions
//

auto to_layout_calculation_data(const LogicItemStore &store, logicitem_id_t logicitem_id)
    -> layout_calculation_data_t {
    return to_layout_calculation_data(store, logicitem_id, store.position(logicitem_id));
}

auto to_layout_calculation_data(const LogicItemStore &store, logicitem_id_t logicitem_id,
                                point_t position) -> layout_calculation_data_t {
    return layout_calculation_data_t {
        .internal_state_count = 0,  // TODO get count when implemented
        .position = position,
        .input_count = store.input_count(logicitem_id),
        .output_count = store.output_count(logicitem_id),
        .orientation = store.orientation(logicitem_id),
        .logicitem_type = store.type(logicitem_id),
    };
}

auto to_logicitem_definition(const LogicItemStore &store,
                             logicitem_id_t logicitem_id) -> LogicItemDefinition {
    return LogicItemDefinition {
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
