#ifndef LOGIKSIM_INDEX_SPATIAL_INDEX_H
#define LOGIKSIM_INDEX_SPATIAL_INDEX_H

#include "format/struct.h"
#include "layout_message_forward.h"
#include "vocabulary/segment_index.h"

#include <array>
#include <cstdint>
#include <span>
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
class Selection;

namespace spatial_index {

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

}  // namespace spatial_index

/**
 * brief: Efficiently stores selection-boxes of inserted Layout elements.
 *
 *  * Pre-conditions:
 *   + requires a correct history of messages of element changes
 */
class SpatialIndex {
   public:
    using value_t = spatial_index::tree_payload_t;
    using queried_segments_t = std::array<segment_t, 4>;

   public:
    explicit SpatialIndex(const Layout &layout);
    explicit SpatialIndex();
    ~SpatialIndex();
    SpatialIndex(const SpatialIndex &);
    auto operator=(const SpatialIndex &) -> SpatialIndex &;
    SpatialIndex(SpatialIndex &&);
    auto operator=(SpatialIndex &&) -> SpatialIndex &;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto operator==(const SpatialIndex &) const -> bool;

    auto query_selection(rect_fine_t rect) const -> std::vector<value_t>;
    auto has_element(point_fine_t point) const -> bool;
    auto query_line_segments(point_t point) const -> queried_segments_t;

    auto rects() const -> std::vector<rect_fine_t>;

    auto submit(const InfoMessage &message) -> void;

   private:
    auto handle(const info_message::LogicItemInserted &message) -> void;
    auto handle(const info_message::LogicItemUninserted &message) -> void;
    auto handle(const info_message::InsertedLogicItemIdUpdated &message) -> void;

    auto handle(const info_message::SegmentInserted &message) -> void;
    auto handle(const info_message::SegmentUninserted &message) -> void;
    auto handle(const info_message::InsertedSegmentIdUpdated &message) -> void;

    // never null
    std::unique_ptr<spatial_index::tree_container> tree_;
};

static_assert(std::regular<SpatialIndex>);

[[nodiscard]] auto get_segment_count(SpatialIndex::queried_segments_t result) -> int;
[[nodiscard]] auto all_same_wire_id(SpatialIndex::queried_segments_t result) -> bool;
[[nodiscard]] auto get_segment_indices(SpatialIndex::queried_segments_t result)
    -> std::array<segment_index_t, 4>;
[[nodiscard]] auto get_unique_wire_id(SpatialIndex::queried_segments_t) -> wire_id_t;

[[nodiscard]] auto is_selected(const SpatialIndex::value_t &item, point_fine_t point,
                               const Selection &selection, const Layout &layout) -> bool;
[[nodiscard]] auto anything_selected(std::span<const SpatialIndex::value_t> items,
                                     point_fine_t point, const Selection &selection,
                                     const Layout &layout) -> bool;
[[nodiscard]] auto all_selected(std::span<const SpatialIndex::value_t> items,
                                point_fine_t point, const Selection &selection,
                                const Layout &layout) -> bool;

}  // namespace logicsim

#endif