
#include "layout.h"

#include "algorithm/range.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "allocated_size/folly_small_vector.h"
#include "allocated_size/std_string.h"
#include "allocated_size/std_vector.h"
#include "allocated_size/trait.h"
#include "exception.h"
#include "layout_calculation.h"
#include "layout_calculation_type.h"
#include "scene.h"
#include "timer.h"

#include <range/v3/algorithm/sort.hpp>
#include <range/v3/view/zip.hpp>

#include <tuple>

namespace logicsim {

namespace layout {
auto attributes_clock_generator::format() const -> std::string {
    return fmt::format("<Clock: '{}', Symmetric: {}, Period: {}, Controls: {}>", name,
                       is_symmetric, format_period(), show_simulation_controls);
}

auto attributes_clock_generator::format_period() const -> std::string {
    if (is_symmetric) {
        const auto period =
            2 * std::chrono::duration<int64_t, delay_t::value_type::period> {
                    time_symmetric.value};
        return fmt::format("{}", format_time(period));
    }
    return fmt::format("{}/{}", time_on, time_off);
}

auto attributes_clock_generator::allocated_size() const -> std::size_t {
    return get_allocated_size(name);
}

auto attributes_clock_generator::is_valid() const -> bool {
    return time_symmetric > delay_t {0ns} && time_on > delay_t {0ns} &&
           time_off > delay_t {0ns};
}

}  // namespace layout

//
// Layout
//

Layout::Layout(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {}

auto Layout::swap(Layout &other) noexcept -> void {
    using std::swap;

    element_types_.swap(other.element_types_);
    sub_circuit_ids_.swap(other.sub_circuit_ids_);
    input_counts_.swap(other.input_counts_);
    output_counts_.swap(other.output_counts_);
    input_inverters_.swap(other.input_inverters_);
    output_inverters_.swap(other.output_inverters_);

    segment_trees_.swap(other.segment_trees_);
    line_trees_.swap(other.line_trees_);
    positions_.swap(other.positions_);
    orientations_.swap(other.orientations_);
    display_states_.swap(other.display_states_);
    bounding_rects_.swap(other.bounding_rects_);

    map_clock_generator_.swap(other.map_clock_generator_);

    swap(circuit_id_, other.circuit_id_);
}

auto Layout::swap_element_data(element_id_t element_id_1, element_id_t element_id_2)
    -> void {
    if (element_id_1 == element_id_2) {
        return;
    }

    const auto swap_ids = [element_id_1, element_id_2](auto &container) {
        using std::swap;
        swap(container.at(element_id_1.value), container.at(element_id_2.value));
    };

    // TODO put in algorithm
    const auto swap_map_ids = [element_id_1, element_id_2](auto &map) {
        auto it1 = map.find(element_id_1);
        auto it2 = map.find(element_id_2);

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
            map.emplace(element_id_2, std::move(tmp));
            return;
        }

        if (it1 == map.end() && it2 != map.end()) {
            auto tmp = std::move(it2->second);
            map.erase(it2);
            map.emplace(element_id_1, std::move(tmp));
            return;
        }

        throw_exception("unknown case in swap_map_ids");
    };

    swap_ids(element_types_);
    swap_ids(sub_circuit_ids_);
    swap_ids(input_counts_);
    swap_ids(output_counts_);
    swap_ids(input_inverters_);
    swap_ids(output_inverters_);

    swap_ids(segment_trees_);
    swap_ids(line_trees_);
    swap_ids(positions_);
    swap_ids(orientations_);
    swap_ids(display_states_);
    swap_ids(bounding_rects_);

    swap_map_ids(map_clock_generator_);
}

auto Layout::delete_last_element() -> void {
    if (empty()) {
        throw_exception("Cannot delete last element of empty schematics.");
    }

    const auto erase_last_map =
        [last_element_id = element_id_t {element_count() - size_t {1}}](auto &map) {
            map.erase(last_element_id);
        };

    element_types_.pop_back();
    sub_circuit_ids_.pop_back();
    input_counts_.pop_back();
    output_counts_.pop_back();
    input_inverters_.pop_back();
    output_inverters_.pop_back();

    segment_trees_.pop_back();
    line_trees_.pop_back();
    positions_.pop_back();
    orientations_.pop_back();
    display_states_.pop_back();
    bounding_rects_.pop_back();

    erase_last_map(map_clock_generator_);
}

auto Layout::allocated_size() const -> std::size_t {
    return get_allocated_size(element_types_) +     //
           get_allocated_size(sub_circuit_ids_) +   //
           get_allocated_size(input_counts_) +      //
           get_allocated_size(output_counts_) +     //
           get_allocated_size(input_inverters_) +   //
           get_allocated_size(output_inverters_) +  //

           get_allocated_size(segment_trees_) +   //
           get_allocated_size(line_trees_) +      //
           get_allocated_size(positions_) +       //
           get_allocated_size(orientations_) +    //
           get_allocated_size(display_states_) +  //
           get_allocated_size(bounding_rects_) +  //

           get_allocated_size(map_clock_generator_);
}

auto Layout::normalize() -> void {
    // reset all caches
    line_trees_ = std::vector<LineTree>(line_trees_.size(), LineTree {});
    bounding_rects_ = std::vector<rect_t>(bounding_rects_.size(), empty_bounding_rect);

    // normallize all members
    for (auto &tree : segment_trees_) {
        tree.normalize();
    }

    // sort our data (except caches)
    const auto vectors = ranges::zip_view(  //
        element_types_,                     //
        sub_circuit_ids_,                   //
        input_counts_,                      //
        output_counts_,                     //
        input_inverters_,                   //
        output_inverters_,                  //
                                            //
        segment_trees_,                     //
        positions_,                         //
        orientations_,                      //
        display_states_                     //
    );

    ranges::sort(vectors);
}

auto Layout::update_bounding_rect(element_id_t element_id) const -> void {
    const auto element = this->element(element_id);
    auto &rect = bounding_rects_.at(element_id.value);

    if (element.is_logic_item()) {
        const auto data = element.to_layout_calculation_data();
        rect = element_collision_rect(data);
    }

    else if (element.is_wire()) {
        const auto &tree = segment_tree(element_id);

        if (tree.empty()) {
            rect = empty_bounding_rect;
        } else {
            rect = calculate_bounding_rect(tree);
        }
    }
}

auto is_inserted(const Layout &layout, element_id_t element_id) -> bool {
    return is_inserted(layout.display_state(element_id));
}

auto get_segment_info(const Layout &layout, segment_t segment) -> segment_info_t {
    return layout.segment_tree(segment.element_id).segment_info(segment.segment_index);
}

auto get_segment_point_type(const Layout &layout, segment_t segment, point_t position)
    -> SegmentPointType {
    const auto info = get_segment_info(layout, segment);

    if (info.line.p0 == position) {
        return info.p0_type;
    } else if (info.line.p1 == position) {
        return info.p1_type;
    };
    throw_exception("Position needs to be an endpoint of the segment.");
}

auto get_line(const Layout &layout, segment_t segment) -> ordered_line_t {
    return get_segment_info(layout, segment).line;
}

auto get_line(const Layout &layout, segment_part_t segment_part) -> ordered_line_t {
    const auto full_line = get_line(layout, segment_part.segment);
    return to_line(full_line, segment_part.part);
}

auto has_segments(const Layout &layout) -> bool {
    for (const auto element_id : layout.element_ids()) {
        if (!layout.segment_tree(element_id).empty()) {
            return true;
        }
    }
    return false;
}

auto moved_layout(Layout layout, int delta_x, int delta_y) -> std::optional<Layout> {
    for (auto element : layout.elements()) {
        if (element.is_logic_item()) {
            if (!is_representable(element.position(), delta_x, delta_y)) {
                return std::nullopt;
            }
            layout.set_position(element,
                                add_unchecked(element.position(), delta_x, delta_y));
        } else if (element.is_wire()) {
            auto &tree = element.modifyable_segment_tree();
            for (const auto segment_index : tree.indices()) {
                auto info = tree.segment_info(segment_index);

                if (!is_representable(info.line, delta_x, delta_y)) {
                    return std::nullopt;
                }

                info.line = add_unchecked(info.line, delta_x, delta_y);
                tree.update_segment(segment_index, info);
            }
        }
    }

    return std::optional {std::move(layout)};
}

auto swap(Layout &a, Layout &b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::Layout &a, logicsim::Layout &b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

auto Layout::format() const -> std::string {
    auto inner = std::string {};

    if (!empty()) {
        const auto lines = fmt_join(",\n  ", elements());
        inner = fmt::format(": [\n  {}\n]", lines);
    }

    return fmt::format("<Layout with {} elements{}>", element_count(), inner);
}

auto Layout::format_stats() const -> std::string {
    auto element_count = std::size_t {0};
    auto segment_count = std::size_t {0};

    for (auto element : elements()) {
        if (element.is_wire()) {
            const auto &tree = element.segment_tree();
            segment_count += tree.segment_count();
        }

        else if (element.is_logic_item()) {
            ++element_count;
        }
    }

    return fmt::format("Layout with {} elements and {} wire segments.\n", element_count,
                       segment_count);
}

auto Layout::empty() const -> bool {
    return positions_.empty();
}

auto Layout::element_count() const -> std::size_t {
    return positions_.size();
}

auto Layout::is_element_id_valid(element_id_t element_id) const noexcept -> bool {
    auto size = gsl::narrow_cast<element_id_t::value_type>(element_count());
    return element_id.value >= 0 && element_id.value < size;
}

auto Layout::add_element(ElementData &&data) -> layout::Element {
    if (element_count() >= std::size_t {element_id_t::max()} - std::size_t {1})
        [[unlikely]] {
        throw_exception("Reached maximum number of elements.");
    }

    const auto element_id =
        element_id_t {gsl::narrow_cast<element_id_t::value_type>(element_types_.size())};

    // extend vectors
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.circuit_id);
    input_counts_.push_back(data.input_count);
    output_counts_.push_back(data.output_count);

    if (data.input_inverters.empty()) {
        input_inverters_.emplace_back(data.input_count.count(), false);
    } else {
        if (connection_count_t {data.input_inverters.size()} != data.input_count)
            [[unlikely]] {
            throw_exception("number of input inverters need to match input count");
        }
        input_inverters_.emplace_back(data.input_inverters);
    }
    if (data.output_inverters.empty()) {
        output_inverters_.emplace_back(data.output_count.count(), false);
    } else {
        if (connection_count_t {data.output_inverters.size()} != data.output_count)
            [[unlikely]] {
            throw_exception("number of output inverters need to match output count");
        }
        output_inverters_.emplace_back(data.output_inverters);
    }

    segment_trees_.emplace_back();
    line_trees_.emplace_back();
    positions_.push_back(data.position);
    orientations_.push_back(data.orientation);
    display_states_.push_back(data.display_state);
    bounding_rects_.push_back(empty_bounding_rect);

    // maps
    const auto add_map_entry = [element_id](auto &map, auto &&optional) {
        if (optional && !map.emplace(element_id, std::move(*optional)).second) {
            throw_exception("element_id already exists in map");
        }
    };

    add_map_entry(map_clock_generator_, std::move(data.attrs_clock_generator));

    return element(element_id);
}

auto Layout::swap_and_delete_element(element_id_t element_id) -> element_id_t {
    const auto last_element_id = element_id_t {
        gsl::narrow_cast<element_id_t::value_type>(element_count() - std::size_t {1})};

    swap_element_data(element_id, last_element_id);
    delete_last_element();
    return last_element_id;
}

auto Layout::swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void {
    swap_element_data(element_id_0, element_id_1);
}

auto Layout::set_position(element_id_t element_id, point_t position) -> void {
    positions_.at(element_id.value) = position;
    bounding_rects_.at(element_id.value) = empty_bounding_rect;
}

auto Layout::set_display_state(element_id_t element_id, display_state_t display_state)
    -> void {
    display_states_.at(element_id.value) = display_state;
}

auto Layout::set_attributes(element_id_t element_id,
                            layout::attributes_clock_generator attrs) -> void {
    const auto it = map_clock_generator_.find(element_id);
    if (it == map_clock_generator_.end()) [[unlikely]] {
        throw_exception("could not find attribute");
    }
    if (!attrs.is_valid()) [[unlikely]] {
        throw_exception("attributes not valid");
    }
    it->second = std::move(attrs);
}

auto Layout::element_ids() const noexcept -> forward_range_t<element_id_t> {
    const auto count =
        element_id_t {gsl::narrow_cast<element_id_t::value_type>(element_count())};
    return range(count);
}

auto Layout::element(element_id_t element_id) -> layout::Element {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return layout::Element {*this, element_id};
}

auto Layout::element(element_id_t element_id) const -> layout::ConstElement {
    if (!is_element_id_valid(element_id)) [[unlikely]] {
        throw_exception("Element id is invalid");
    }
    return layout::ConstElement {*this, element_id};
}

auto Layout::circuit_id() const noexcept -> circuit_id_t {
    return circuit_id_;
}

auto Layout::element_type(element_id_t element_id) const -> ElementType {
    return element_types_.at(element_id.value);
}

auto Layout::sub_circuit_id(element_id_t element_id) const -> circuit_id_t {
    return sub_circuit_ids_.at(element_id.value);
}

auto Layout::input_count(element_id_t element_id) const -> connection_count_t {
    return connection_count_t {input_counts_.at(element_id.value)};
}

auto Layout::output_count(element_id_t element_id) const -> connection_count_t {
    return connection_count_t {output_counts_.at(element_id.value)};
}

auto Layout::input_inverters(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return input_inverters_.at(element_id.value);
}

auto Layout::output_inverters(element_id_t element_id) const
    -> const logic_small_vector_t & {
    return output_inverters_.at(element_id.value);
}

auto Layout::segment_tree(element_id_t element_id) const -> const SegmentTree & {
    return segment_trees_.at(element_id.value);
}

auto Layout::line_tree(element_id_t element_id) const -> const LineTree & {
    auto &line_tree = line_trees_.at(element_id.value);
    auto element = this->element(element_id);

    if (line_tree.empty() && element.is_wire() &&
        element.display_state() == display_state_t::normal &&
        element.segment_tree().has_input()) {
        line_tree = to_line_tree(element.segment_tree()).value();

        if (line_tree.empty()) {
            throw_exception("generated line tree is empty");
        }
    }

    return line_tree;
}

auto Layout::position(element_id_t element_id) const -> point_t {
    return positions_.at(element_id.value);
}

auto Layout::orientation(element_id_t element_id) const -> orientation_t {
    return orientations_.at(element_id.value);
}

auto Layout::display_state(element_id_t element_id) const -> display_state_t {
    return display_states_.at(element_id.value);
}

auto Layout::bounding_rect(element_id_t element_id) const -> rect_t {
    if (bounding_rects_.at(element_id.value) == empty_bounding_rect) {
        update_bounding_rect(element_id);
    }
    return bounding_rects_.at(element_id.value);
}

auto Layout::attrs_clock_generator(element_id_t element_id) const
    -> const layout::attributes_clock_generator & {
    const auto it = map_clock_generator_.find(element_id);
    if (it == map_clock_generator_.end()) {
        throw_exception("could not find attribute");
    }
    return it->second;
}

auto Layout::modifyable_segment_tree(element_id_t element_id) -> SegmentTree & {
    // reset line tree
    line_trees_.at(element_id.value) = LineTree {};
    // reset bounding rect
    bounding_rects_.at(element_id.value) = empty_bounding_rect;

    return segment_trees_.at(element_id.value);
}

auto validate_segment_tree_display_state(const SegmentTree &tree,
                                         display_state_t display_state) -> void {
    if (!tree.empty()) {
        bool any_valid_parts =
            std::ranges::any_of(tree.valid_parts(), &SegmentTree::parts_vector_t::size);

        if (any_valid_parts && !is_inserted(display_state)) [[unlikely]] {
            throw_exception("segment tree is in the wrong display state");
        }
    }
}

auto Layout::validate() const -> void {
    // wires
    const auto validate_segment_tree = [&](element_id_t element_id) {
        if (is_inserted(*this, element_id) && !segment_tree(element_id).empty()) {
            segment_tree(element_id).validate_inserted();
        } else {
            segment_tree(element_id).validate();
        }
    };
    for (const auto element_id : element_ids()) {
        line_tree(element_id).validate();
        validate_segment_tree(element_id);
        validate_segment_tree_display_state(segment_tree(element_id),
                                            display_state(element_id));
    }

    // global attributes
    if (!circuit_id_) [[unlikely]] {
        throw_exception("invalid circuit id");
    }
}

//
// Element Template
//

namespace layout {
template <bool Const>
inline ElementTemplate<Const>::ElementTemplate(LayoutType &layout,
                                               element_id_t element_id) noexcept
    : layout_(&layout), element_id_(element_id) {}

template <bool Const>
template <bool ConstOther>
ElementTemplate<Const>::ElementTemplate(ElementTemplate<ConstOther> element) noexcept
    requires Const && (!ConstOther)
    : layout_(element.layout_), element_id_(element.element_id_) {}

template <bool Const>
ElementTemplate<Const>::operator element_id_t() const noexcept {
    return element_id_;
}

template <bool Const>
template <bool ConstOther>
auto ElementTemplate<Const>::operator==(ElementTemplate<ConstOther> other) const noexcept
    -> bool {
    return layout_ == other.layout_ && element_id_ == other.element_id_;
}

template <bool Const>
auto ElementTemplate<Const>::format() const -> std::string {
    const auto info = [&]() {
        if (is_wire()) {
            return fmt::format("{}-{}", display_state(), segment_tree());
        }
        return fmt::format("{}x{} {}, {}, {}, {}", input_count(), output_count(),
                           element_type(), display_state(), position(), orientation());
    };

    return fmt::format("<Element {}: {}>", element_id(), info());
}

template <bool Const>
auto ElementTemplate<Const>::to_layout_calculation_data() const
    -> layout_calculation_data_t {
    return logicsim::to_layout_calculation_data(layout(), element_id());
}

template <bool Const>
auto ElementTemplate<Const>::layout() const noexcept -> LayoutType & {
    return *layout_;
}

template <bool Const>
auto ElementTemplate<Const>::element_id() const noexcept -> element_id_t {
    return element_id_;
}

template <bool Const>
auto ElementTemplate<Const>::sub_circuit_id() const -> circuit_id_t {
    return layout_->sub_circuit_id(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::element_type() const -> ElementType {
    return layout_->element_type(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::is_unused() const -> bool {
    return element_type() == ElementType::unused;
}

template <bool Const>
auto ElementTemplate<Const>::is_placeholder() const -> bool {
    return element_type() == ElementType::placeholder;
}

template <bool Const>
auto ElementTemplate<Const>::is_wire() const -> bool {
    return element_type() == ElementType::wire;
}

template <bool Const>
auto ElementTemplate<Const>::is_logic_item() const -> bool {
    return ::logicsim::is_logic_item(element_type());
}

template <bool Const>
auto ElementTemplate<Const>::is_sub_circuit() const -> bool {
    return element_type() == ElementType::sub_circuit;
}

template <bool Const>
auto ElementTemplate<Const>::input_count() const -> connection_count_t {
    return layout_->input_count(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::output_count() const -> connection_count_t {
    return layout_->output_count(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::input_inverters() const -> const logic_small_vector_t & {
    return layout_->input_inverters(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::output_inverters() const -> const logic_small_vector_t & {
    return layout_->output_inverters(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::input_inverted(connection_id_t index) const -> bool {
    return input_inverters().at(index.value);
}

template <bool Const>
auto ElementTemplate<Const>::output_inverted(connection_id_t index) const -> bool {
    return output_inverters().at(index.value);
}

template <bool Const>
auto ElementTemplate<Const>::segment_tree() const -> const SegmentTree & {
    return layout_->segment_tree(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::line_tree() const -> const LineTree & {
    return layout_->line_tree(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::position() const -> point_t {
    return layout_->position(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::orientation() const -> orientation_t {
    return layout_->orientation(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::display_state() const -> display_state_t {
    return layout_->display_state(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::is_inserted() const -> bool {
    return logicsim::is_inserted(this->display_state());
}

template <bool Const>
auto ElementTemplate<Const>::bounding_rect() const -> rect_t {
    return layout_->bounding_rect(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::attrs_clock_generator() const
    -> const layout::attributes_clock_generator & {
    return layout_->attrs_clock_generator(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::modifyable_segment_tree() const -> SegmentTree &
    requires(!Const)
{
    return layout_->modifyable_segment_tree(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::set_input_inverter(connection_id_t index, bool value) const
    -> void
    requires(!Const)
{
    layout_->input_inverters_.at(element_id_.value).at(index.value) = value;
}

template <bool Const>
auto ElementTemplate<Const>::set_output_inverter(connection_id_t index, bool value) const
    -> void
    requires(!Const)
{
    layout_->output_inverters_.at(element_id_.value).at(index.value) = value;
}

// Template Instanciations

template class ElementTemplate<true>;
template class ElementTemplate<false>;

template ElementTemplate<true>::ElementTemplate(ElementTemplate<false>) noexcept;

template auto ElementTemplate<false>::operator==
    <false>(ElementTemplate<false>) const noexcept -> bool;
template auto ElementTemplate<false>::operator==
    <true>(ElementTemplate<true>) const noexcept -> bool;
template auto ElementTemplate<true>::operator==
    <false>(ElementTemplate<false>) const noexcept -> bool;
template auto ElementTemplate<true>::operator==
    <true>(ElementTemplate<true>) const noexcept -> bool;

}  // namespace layout

auto to_layout_calculation_data(const Layout &layout, element_id_t element_id)
    -> layout_calculation_data_t {
    const auto element = layout.element(element_id);

    return layout_calculation_data_t {
        .internal_state_count = 0,  // TODO get count fromm schematic when implemented
        .position = element.position(),
        .input_count = element.input_count(),
        .output_count = element.output_count(),
        .orientation = element.orientation(),
        .element_type = element.element_type(),
    };
}

}  // namespace logicsim
