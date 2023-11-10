#ifndef LOGICSIM_LINE_TREE_H
#define LOGICSIM_LINE_TREE_H

#include "algorithm/range_extended.h"
#include "component/line_tree/line_store.h"
#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/length_vector.h"
#include "vocabulary/orientation.h"

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
    [[nodiscard]] auto operator==(const LineTree &) const -> bool = default;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto begin() const -> iterator;
    [[nodiscard]] auto end() const -> iterator;

    [[nodiscard]] auto lines() const -> std::span<const line_t>;
    [[nodiscard]] auto line(line_index_t) const -> line_t;
    [[nodiscard]] auto has_cross_point_p0(line_index_t) const -> bool;
    [[nodiscard]] auto is_corner_p0(line_index_t) const -> bool;
    [[nodiscard]] auto is_corner_p1(line_index_t) const -> bool;
    [[nodiscard]] auto length_p0(line_index_t) const -> length_t;
    [[nodiscard]] auto length_p1(line_index_t) const -> length_t;

    [[nodiscard]] auto input_position() const -> point_t;
    [[nodiscard]] auto input_orientation() const -> orientation_t;

    [[nodiscard]] auto output_count() const -> connection_count_t;
    [[nodiscard]] auto output_position(connection_id_t) const -> point_t;
    [[nodiscard]] auto output_orientation(connection_id_t) const -> orientation_t;
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

}  // namespace logicsim

#endif
