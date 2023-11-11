#ifndef LOGIKSIM_EDITABLE_CIRCUIT_CACHE_SPATIAL_CACHE_H
#define LOGIKSIM_EDITABLE_CIRCUIT_CACHE_SPATIAL_CACHE_H

#include "editable_circuit/message_forward.h"
#include "format/struct.h"
#include "vocabulary/segment_index.h"

#include <array>
#include <cstdint>
#include <vector>

namespace logicsim {

struct point_t;
struct point_fine_t;
struct rect_fine_t;

struct logicitem_id_t;
struct wire_id_t;
struct segment_index_t;
struct segment_t;

struct layout_calculation_data_t;
class Layout;

namespace detail::spatial_tree {

struct tree_payload_t {
    [[nodiscard]] explicit tree_payload_t(logicitem_id_t logicitem_id);
    [[nodiscard]] explicit tree_payload_t(segment_t segment);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto hash() const -> uint64_t;

    [[nodiscard]] auto is_logicitem() const -> bool;
    [[nodiscard]] auto logicitem() const -> logicitem_id_t;

    [[nodiscard]] auto is_segment() const -> bool;
    [[nodiscard]] auto segment() const -> segment_t;

    [[nodiscard]] auto operator==(const tree_payload_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const tree_payload_t &other) const = default;

   private:
    // logicitem_id_t | wire_id_t
    int32_t element_id_;
    segment_index_t segment_index_;
};

struct tree_container;

}  // namespace detail::spatial_tree

class SpatialTree {
   public:
    using value_t = detail::spatial_tree::tree_payload_t;
    using queried_segments_t = std::array<segment_t, 4>;

   public:
    explicit SpatialTree();
    ~SpatialTree();
    SpatialTree(SpatialTree &&);
    auto operator=(SpatialTree &&) -> SpatialTree &;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto query_selection(rect_fine_t rect) const -> std::vector<value_t>;
    auto has_element(point_fine_t point) const -> bool;
    auto query_line_segments(point_t point) const -> queried_segments_t;

    auto rects() const -> std::vector<rect_fine_t>;

    auto submit(const editable_circuit::InfoMessage &message) -> void;
    auto validate(const Layout &layout) const -> void;

   private:
    auto handle(const editable_circuit::info_message::LogicItemInserted &message) -> void;
    auto handle(const editable_circuit::info_message::LogicItemUninserted &message)
        -> void;
    auto handle(const editable_circuit::info_message::InsertedLogicItemIdUpdated &message)
        -> void;

    auto handle(const editable_circuit::info_message::SegmentInserted &message) -> void;
    auto handle(const editable_circuit::info_message::SegmentUninserted &message) -> void;
    auto handle(const editable_circuit::info_message::InsertedSegmentIdUpdated &message)
        -> void;

    std::unique_ptr<detail::spatial_tree::tree_container> tree_;
};

[[nodiscard]] auto get_segment_count(SpatialTree::queried_segments_t result) -> int;
[[nodiscard]] auto all_same_wire_id(SpatialTree::queried_segments_t result) -> bool;
[[nodiscard]] auto get_segment_indices(SpatialTree::queried_segments_t result)
    -> std::array<segment_index_t, 4>;
[[nodiscard]] auto get_unique_wire_id(SpatialTree::queried_segments_t) -> wire_id_t;

}  // namespace logicsim

#endif