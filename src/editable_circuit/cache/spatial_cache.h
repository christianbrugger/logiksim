#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_SPATIAL_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_SPATIAL_CACHE_H

#include "editable_circuit/message_forward.h"
#include "vocabulary.h"

#include <gsl/gsl>

namespace boost::geometry::model::d2 {
template <typename CoordinateType, typename CoordinateSystem>
class point_xy;
}

namespace boost::geometry::index {}

namespace logicsim {

class Layout;
struct layout_calculation_data_t;

namespace detail::spatial_tree {

struct tree_payload_t {
    element_id_t element_id {null_element};
    segment_index_t segment_index {null_segment_index};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const tree_payload_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const tree_payload_t &other) const = default;
};

struct tree_container;

}  // namespace detail::spatial_tree

class SpatialTree {
   public:
    using query_result_t = detail::spatial_tree::tree_payload_t;
    using queried_segments_t = std::array<segment_t, 4>;

   public:
    explicit SpatialTree();
    ~SpatialTree();
    SpatialTree(SpatialTree &&);
    auto operator=(SpatialTree &&) -> SpatialTree &;

    [[nodiscard]] auto format() const -> std::string;

    auto query_selection(rect_fine_t rect) const -> std::vector<query_result_t>;
    auto has_element(point_fine_t point) const -> bool;
    auto query_line_segments(point_t point) const -> queried_segments_t;

    auto rects() const -> std::vector<rect_fine_t>;

    auto submit(editable_circuit::InfoMessage message) -> void;
    auto validate(const Layout &layout) const -> void;

   private:
    auto handle(editable_circuit::info_message::LogicItemInserted message) -> void;
    auto handle(editable_circuit::info_message::LogicItemUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedLogicItemIdUpdated message)
        -> void;

    auto handle(editable_circuit::info_message::SegmentInserted message) -> void;
    auto handle(editable_circuit::info_message::SegmentUninserted message) -> void;
    auto handle(editable_circuit::info_message::InsertedSegmentIdUpdated message) -> void;

    std::unique_ptr<detail::spatial_tree::tree_container> tree_;
};

[[nodiscard]] auto get_segment_count(SpatialTree::queried_segments_t result) -> int;
[[nodiscard]] auto all_same_element_id(SpatialTree::queried_segments_t result) -> bool;
[[nodiscard]] auto get_segment_indices(SpatialTree::queried_segments_t result)
    -> std::array<segment_index_t, 4>;
[[nodiscard]] auto get_unique_element_id(SpatialTree::queried_segments_t) -> element_id_t;

}  // namespace logicsim

#endif