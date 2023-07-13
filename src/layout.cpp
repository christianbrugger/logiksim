
#include "layout.h"

#include "exceptions.h"
#include "iterator_adaptor.h"
#include "layout_calculation_type.h"
#include "range.h"

namespace logicsim {

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
    orientation_.swap(other.orientation_);
    display_states_.swap(other.display_states_);
    colors_.swap(other.colors_);

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

    swap_ids(element_types_);
    swap_ids(sub_circuit_ids_);
    swap_ids(input_counts_);
    swap_ids(output_counts_);
    swap_ids(input_inverters_);
    swap_ids(output_inverters_);

    swap_ids(segment_trees_);
    swap_ids(line_trees_);
    swap_ids(positions_);
    swap_ids(orientation_);
    swap_ids(display_states_);
    swap_ids(colors_);
}

auto Layout::delete_last_element() -> void {
    if (empty()) {
        throw_exception("Cannot delete last element of empty schematics.");
    }

    element_types_.pop_back();
    sub_circuit_ids_.pop_back();
    input_counts_.pop_back();
    output_counts_.pop_back();
    input_inverters_.pop_back();
    output_inverters_.pop_back();

    segment_trees_.pop_back();
    line_trees_.pop_back();
    positions_.pop_back();
    orientation_.pop_back();
    display_states_.pop_back();
    colors_.pop_back();
}

auto is_inserted(const Layout &layout, element_id_t element_id) -> bool {
    return is_inserted(layout.display_state(element_id));
}

auto get_segment_info(const Layout &layout, segment_t segment) -> segment_info_t {
    return layout.segment_tree(segment.element_id).segment_info(segment.segment_index);
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
    if (data.input_count > connection_id_t::max()) [[unlikely]] {
        throw_exception("Input count needs to be positive and not too large.");
    }
    if (data.output_count > connection_id_t::max()) [[unlikely]] {
        throw_exception("Output count needs to be positive and not too large.");
    }

    if (element_count() + 1 >= element_id_t::max()) [[unlikely]] {
        throw_exception("Reached maximum number of elements.");
    }

    // extend vectors
    element_types_.push_back(data.element_type);
    sub_circuit_ids_.push_back(data.circuit_id);
    input_counts_.push_back(gsl::narrow_cast<connection_size_t>(data.input_count));
    output_counts_.push_back(gsl::narrow_cast<connection_size_t>(data.output_count));
    input_inverters_.emplace_back(data.input_count, false);
    output_inverters_.emplace_back(data.output_count, false);

    segment_trees_.emplace_back();
    line_trees_.emplace_back();
    positions_.push_back(data.position);
    orientation_.push_back(data.orientation);
    display_states_.push_back(data.display_state);
    colors_.push_back(data.color);

    auto element_id = element_id_t {gsl::narrow_cast<element_id_t::value_type>(
        element_types_.size() - std::size_t {1})};
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

auto Layout::set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void {
    line_trees_.at(element_id.value) = std::move(line_tree);
}

auto Layout::set_position(element_id_t element_id, point_t position) -> void {
    positions_.at(element_id.value) = position;
}

auto Layout::set_display_state(element_id_t element_id, display_state_t display_state)
    -> void {
    display_states_.at(element_id.value) = display_state;
}

auto Layout::element_ids() const noexcept -> forward_range_t<element_id_t> {
    const auto count
        = element_id_t {gsl::narrow_cast<element_id_t::value_type>(element_count())};
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

auto Layout::input_count(element_id_t element_id) const -> std::size_t {
    return input_counts_.at(element_id.value);
}

auto Layout::output_count(element_id_t element_id) const -> std::size_t {
    return output_counts_.at(element_id.value);
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

    if (line_tree.empty() && element.display_state() == display_state_t::normal
        && element.is_wire() && element.segment_tree().has_input()) {
        line_tree = LineTree::from_segment_tree(element.segment_tree()).value();

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
    return orientation_.at(element_id.value);
}

auto Layout::display_state(element_id_t element_id) const -> display_state_t {
    return display_states_.at(element_id.value);
}

auto Layout::color(element_id_t element_id) const -> color_t {
    return colors_.at(element_id.value);
}

auto Layout::modifyable_segment_tree(element_id_t element_id) -> SegmentTree & {
    // reset line tree
    line_trees_.at(element_id.value) = LineTree {};

    return segment_trees_.at(element_id.value);
}

auto validate_segment_tree_display_state(const SegmentTree &tree,
                                         display_state_t display_state) -> void {
    if (!tree.empty()) {
        bool any_valid_parts
            = std::ranges::any_of(tree.valid_parts(), &SegmentTree::parts_vector_t::size);

        if (any_valid_parts && !is_inserted(display_state)) [[unlikely]] {
            throw_exception("segment tree is in the wrong display state");
        }
    }
}

auto Layout::validate() const -> void {
    const auto validate_segment_tree = [&](element_id_t element_id) {
        if (is_inserted(*this, element_id) && !segment_tree(element_id).empty()) {
            segment_tree(element_id).validate_inserted();
        } else {
            segment_tree(element_id).validate();
        }
    };

    // wires
    std::ranges::for_each(line_trees_, &LineTree::validate);
    std::ranges::for_each(element_ids(), validate_segment_tree);
    for (const auto element_id : element_ids()) {
        validate_segment_tree_display_state(segment_tree(element_id),
                                            display_state(element_id));
    }

    // global attributes
    if (!circuit_id_) [[unlikely]] {
        throw_exception("invalid circuit id");
    }
}

//
// Layout
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
    return !(is_unused() || is_placeholder() || is_wire());
}

template <bool Const>
auto ElementTemplate<Const>::is_sub_circuit() const -> bool {
    return element_type() == ElementType::sub_circuit;
}

template <bool Const>
auto ElementTemplate<Const>::input_count() const -> std::size_t {
    return layout_->input_count(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::output_count() const -> std::size_t {
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
auto ElementTemplate<Const>::color() const -> color_t {
    return layout_->color(element_id_);
}

template <bool Const>
auto ElementTemplate<Const>::modifyable_segment_tree() const -> SegmentTree &
    requires(!Const)
{
    return layout_->modifyable_segment_tree(element_id_);
}

// Template Instanciations

template class ElementTemplate<true>;
template class ElementTemplate<false>;

template ElementTemplate<true>::ElementTemplate(ElementTemplate<false>) noexcept;

template auto ElementTemplate<false>::operator==<false>(
    ElementTemplate<false>) const noexcept -> bool;
template auto ElementTemplate<false>::operator==<true>(
    ElementTemplate<true>) const noexcept -> bool;
template auto ElementTemplate<true>::operator==<false>(
    ElementTemplate<false>) const noexcept -> bool;
template auto ElementTemplate<true>::operator==<true>(
    ElementTemplate<true>) const noexcept -> bool;

}  // namespace layout

}  // namespace logicsim
