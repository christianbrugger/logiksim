#ifndef LOGICSIM_LINE_TREE_H
#define LOGICSIM_LINE_TREE_H

#include "core/algorithm/range_extended.h"
#include "core/component/line_tree/line_store.h"
#include "core/format/struct.h"
#include "core/vocabulary/connection_id.h"
#include "core/vocabulary/length_vector.h"
#include "core/vocabulary/orientation.h"

#include <optional>
#include <span>
#include <string>

namespace logicsim {

struct ordered_line_t;
struct connection_count_t;

class LineTree {
    using value_type = line_t;
    using iterator = line_tree::line_vector_t::const_iterator;

   public:
    [[nodiscard]] explicit constexpr LineTree() = default;
    [[nodiscard]] explicit LineTree(line_tree::LineStore &&store);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const LineTree &other) const -> bool = default;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto begin() const -> iterator;
    [[nodiscard]] auto end() const -> iterator;

    [[nodiscard]] auto lines() const -> std::span<const line_t>;
    [[nodiscard]] auto line(line_index_t index) const -> line_t;
    /**
     * @brief: Indicates if there is a cross-point at p0.
     */
    [[nodiscard]] auto has_cross_point_p0(line_index_t index) const -> bool;
    /**
     * @brief: Indicates if there is a corner at the point.
     *
     * Note that currently also cross-points are flagged as corners for some lines.
     */
    [[nodiscard]] auto is_corner_p0(line_index_t index) const -> bool;
    [[nodiscard]] auto is_corner_p1(line_index_t index) const -> bool;
    [[nodiscard]] auto length_p0(line_index_t index) const -> length_t;
    [[nodiscard]] auto length_p1(line_index_t index) const -> length_t;

    [[nodiscard]] auto input_position() const -> point_t;
    [[nodiscard]] auto input_orientation() const -> orientation_t;

    [[nodiscard]] auto output_count() const -> connection_count_t;
    [[nodiscard]] auto output_position(connection_id_t output) const -> point_t;
    [[nodiscard]] auto output_orientation(connection_id_t output) const -> orientation_t;
    [[nodiscard]] auto calculate_output_lengths() const -> length_vector_t;

   private:
    line_tree::LineStore store_;
};

/**
 * @brief: Generates a line tree from list of segments.
 *
 * Pre-condition: segments are expected to form a contiguous tree.
 */
[[nodiscard]] auto to_line_tree(std::span<const ordered_line_t> segments, point_t root)
    -> LineTree;

[[nodiscard]] auto indices(const LineTree &line_tree) -> range_extended_t<line_index_t>;

[[nodiscard]] auto output_ids(const LineTree &line_tree)
    -> range_extended_t<connection_id_t>;

[[nodiscard]] auto format_entry(const LineTree &line_tree, line_index_t index)
    -> std::string;

}  // namespace logicsim

#endif
