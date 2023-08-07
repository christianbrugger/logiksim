#ifndef LOGIKSIM_LAYOUT_H
#define LOGIKSIM_LAYOUT_H

#include "iterator_adaptor.h"
#include "layout_calculation_type.h"
#include "line_tree.h"
#include "segment_tree.h"
#include "vocabulary.h"

#include <fmt/core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace logicsim {

class Layout;

[[nodiscard]] auto is_inserted(const Layout &layout, element_id_t element_id) -> bool;

[[nodiscard]] auto get_segment_info(const Layout &layout, segment_t segment)
    -> segment_info_t;
[[nodiscard]] auto get_segment_point_type(const Layout &layout, segment_t segment,
                                          point_t position) -> SegmentPointType;
[[nodiscard]] auto get_line(const Layout &layout, segment_t segment) -> ordered_line_t;
[[nodiscard]] auto get_line(const Layout &layout, segment_part_t segment_part)
    -> ordered_line_t;

[[nodiscard]] auto has_segments(const Layout &layout) -> bool;

[[nodiscard]] auto moved_layout(Layout layout, int delta_x, int delta_y)
    -> std::optional<Layout>;

namespace layout {
template <bool Const>
class ElementTemplate;

using Element = ElementTemplate<false>;
using ConstElement = ElementTemplate<true>;
}  // namespace layout

class Layout {
    template <bool Const>
    friend class layout::ElementTemplate;

   public:
    [[nodiscard]] explicit Layout() = default;
    [[nodiscard]] explicit Layout(circuit_id_t circuit_id);

    auto swap(Layout &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;
    auto normalize() -> void;  // bring it into a form that can be compared

    [[nodiscard]] auto operator==(const Layout &) const -> bool = default;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto element_count() const -> std::size_t;
    [[nodiscard]] auto is_element_id_valid(element_id_t element_id) const noexcept
        -> bool;

    struct ElementData {
        display_state_t display_state {display_state_t::temporary};
        ElementType element_type {ElementType::unused};

        std::size_t input_count {0};
        std::size_t output_count {0};
        point_t position {point_t {0, 0}};
        orientation_t orientation {orientation_t::undirected};
        color_t color {defaults::color_black};

        circuit_id_t circuit_id {null_circuit};
        logic_small_vector_t input_inverters {};
        logic_small_vector_t output_inverters {};
    };

    auto add_element(ElementData &&data) -> layout::Element;

    // swaps the element with last one and deletes it, returns deleted id
    auto swap_and_delete_element(element_id_t element_id) -> element_id_t;
    auto swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void;

    // TODO remove line tree, when not needed anymore
    auto set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void;
    auto set_position(element_id_t element_id, point_t point) -> void;
    auto set_display_state(element_id_t element_id, display_state_t display_state)
        -> void;

    [[nodiscard]] auto circuit_id() const noexcept -> circuit_id_t;
    [[nodiscard]] auto element_ids() const noexcept -> forward_range_t<element_id_t>;

    [[nodiscard]] auto element(element_id_t element_id) -> layout::Element;
    [[nodiscard]] auto element(element_id_t element_id) const -> layout::ConstElement;
    [[nodiscard]] inline auto elements();
    [[nodiscard]] inline auto elements() const;

    [[nodiscard]] auto element_type(element_id_t element_id) const -> ElementType;
    [[nodiscard]] auto sub_circuit_id(element_id_t element_id) const -> circuit_id_t;
    [[nodiscard]] auto input_count(element_id_t element_id) const -> std::size_t;
    [[nodiscard]] auto output_count(element_id_t element_id) const -> std::size_t;
    [[nodiscard]] auto input_inverters(element_id_t element_id) const
        -> const logic_small_vector_t &;
    [[nodiscard]] auto output_inverters(element_id_t element_id) const
        -> const logic_small_vector_t &;

    [[nodiscard]] auto segment_tree(element_id_t element_id) const -> const SegmentTree &;
    [[nodiscard]] auto line_tree(element_id_t element_id) const -> const LineTree &;
    [[nodiscard]] auto position(element_id_t element_id) const -> point_t;
    [[nodiscard]] auto orientation(element_id_t element_id) const -> orientation_t;
    [[nodiscard]] auto display_state(element_id_t element_id) const -> display_state_t;
    [[nodiscard]] auto color(element_id_t element_id) const -> color_t;
    [[nodiscard]] auto bounding_rect(element_id_t element_id) const -> rect_t;

    [[nodiscard]] auto modifyable_segment_tree(element_id_t element_id) -> SegmentTree &;

    auto validate() const -> void;

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2) -> void;
    auto delete_last_element() -> void;
    auto update_bounding_rect(element_id_t element_id) const -> void;

   private:
    constexpr static auto empty_bounding_rect = rect_t {point_t {0, 0}, point_t {0, 0}};

    using connection_size_t = std::make_unsigned<connection_id_t::value_type>::type;
    static_assert(sizeof(connection_size_t) == sizeof(connection_id_t));

    std::vector<ElementType> element_types_ {};
    std::vector<circuit_id_t> sub_circuit_ids_ {};
    std::vector<connection_size_t> input_counts_ {};
    std::vector<connection_size_t> output_counts_ {};
    std::vector<logic_small_vector_t> input_inverters_ {};
    std::vector<logic_small_vector_t> output_inverters_ {};

    std::vector<SegmentTree> segment_trees_ {};
    mutable std::vector<LineTree> line_trees_ {};
    std::vector<point_t> positions_ {};
    std::vector<orientation_t> orientations_ {};
    std::vector<display_state_t> display_states_ {};
    std::vector<color_t> colors_ {};

    mutable std::vector<rect_t> bounding_rects_ {};

    circuit_id_t circuit_id_ {0};
};

auto swap(Layout &a, Layout &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Layout &a, logicsim::Layout &b) noexcept -> void;

namespace logicsim {

namespace layout {

template <bool Const>
class ElementTemplate {
    using LayoutType = std::conditional_t<Const, const Layout, Layout>;

    friend ElementTemplate<!Const>;
    friend LayoutType;

    explicit ElementTemplate(LayoutType &layout, element_id_t element_id) noexcept;

   public:
    /// This constructor is not regarded as a copy constructor,
    //   we preserve trivially copyable
    template <bool ConstOther>
    // NOLINTNEXTLINE(google-explicit-constructor)
    ElementTemplate(ElementTemplate<ConstOther> element) noexcept
        requires Const && (!ConstOther);

    [[nodiscard]] operator element_id_t() const noexcept;

    template <bool ConstOther>
    auto operator==(ElementTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto to_layout_calculation_data() const -> layout_calculation_data_t;

    [[nodiscard]] auto layout() const noexcept -> LayoutType &;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;
    [[nodiscard]] auto sub_circuit_id() const -> circuit_id_t;

    [[nodiscard]] auto element_type() const -> ElementType;
    [[nodiscard]] auto is_unused() const -> bool;
    [[nodiscard]] auto is_placeholder() const -> bool;
    [[nodiscard]] auto is_wire() const -> bool;
    [[nodiscard]] auto is_logic_item() const -> bool;
    [[nodiscard]] auto is_sub_circuit() const -> bool;

    [[nodiscard]] auto display_state() const -> display_state_t;
    [[nodiscard]] auto is_inserted() const -> bool;

    [[nodiscard]] auto input_count() const -> std::size_t;
    [[nodiscard]] auto output_count() const -> std::size_t;
    [[nodiscard]] auto input_inverters() const -> const logic_small_vector_t &;
    [[nodiscard]] auto output_inverters() const -> const logic_small_vector_t &;
    [[nodiscard]] auto input_inverted(connection_id_t index) const -> bool;
    [[nodiscard]] auto output_inverted(connection_id_t index) const -> bool;

    [[nodiscard]] auto segment_tree() const -> const SegmentTree &;
    [[nodiscard]] auto line_tree() const -> const LineTree &;
    [[nodiscard]] auto position() const -> point_t;
    [[nodiscard]] auto orientation() const -> orientation_t;
    [[nodiscard]] auto color() const -> color_t;

    [[nodiscard]] auto bounding_rect() const -> rect_t;

    // modification
    [[nodiscard]] auto modifyable_segment_tree() const -> SegmentTree &
        requires(!Const);
    auto set_input_inverter(connection_id_t index, bool value) const -> void
        requires(!Const);
    auto set_output_inverter(connection_id_t index, bool value) const -> void
        requires(!Const);

   private:
    gsl::not_null<LayoutType *> layout_;
    element_id_t element_id_;
};

}  // namespace layout

inline auto Layout::elements() {
    return transform_view(element_ids(), [&](element_id_t element_id) {
        return this->element(element_id);
    });
}

inline auto Layout::elements() const {
    return transform_view(element_ids(), [&](element_id_t element_id) {
        return this->element(element_id);
    });
}

}  // namespace logicsim

#endif