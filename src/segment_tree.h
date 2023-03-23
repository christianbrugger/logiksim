#ifndef LOGIKSIM_SEGMENT_TREE_H
#define LOGIKSIM_SEGMENT_TREE_H

#include "format.h"
#include "vocabulary.h"

#include <folly/small_vector.h>

#include <optional>
#include <span>
#include <vector>

// Open issues:
// - How to deal with segment that connects two outputs: output < ---- > output

namespace logicsim {

enum class SegmentPointType : uint8_t {
    // has collision
    input,
    output,
    colliding_point,

    // no collision
    shadow_point,
    cross_point,
    // unknown state
    new_unknown,
};

auto format(SegmentPointType type) -> std::string;

struct segment_info_t {
    line_t line {};

    SegmentPointType p0_type {SegmentPointType::colliding_point};
    SegmentPointType p1_type {SegmentPointType::colliding_point};

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(sizeof(segment_info_t) == 10);

class SegmentTree {
   public:
    // TODO use segment_index_t
    using index_t = uint16_t;

   public:
    [[nodiscard]] constexpr SegmentTree() = default;

    auto swap(SegmentTree &other) noexcept -> void;

    auto add_segment(segment_info_t segment, display_state_t display_state)
        -> segment_index_t;
    auto add_tree(const SegmentTree &tree) -> segment_index_t;
    auto update_segment(segment_index_t index, segment_info_t segment,
                        display_state_t display_state) -> void;
    // swaps the element with last one and deletes it
    auto swap_and_delete_segment(segment_index_t index) -> void;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto segment_count() const noexcept -> std::size_t;
    [[nodiscard]] auto segment(std::size_t index) const -> segment_info_t;
    [[nodiscard]] auto segment(segment_index_t index) const -> segment_info_t;
    [[nodiscard]] auto segments() const -> std::span<const segment_info_t>;
    [[nodiscard]] auto display_state(segment_index_t index) const -> display_state_t;

    [[nodiscard]] auto first_index() const noexcept -> segment_index_t;
    [[nodiscard]] auto last_index() const noexcept -> segment_index_t;

    [[nodiscard]] auto has_input() const noexcept -> bool;
    [[nodiscard]] auto input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto input_position() const -> point_t;

    [[nodiscard]] auto output_count() const noexcept -> std::size_t;

    [[nodiscard]] auto format() const -> std::string;
    auto validate() const -> void;

   private:
    auto get_next_index() const -> segment_index_t;
    auto register_segment(segment_index_t index) -> void;
    auto unregister_segment(segment_index_t index) -> void;
    auto delete_last_segment() -> void;

   private:
    using policy = folly::small_vector_policy::policy_size_type<index_t>;
    using segment_vector_t = folly::small_vector<segment_info_t, 2, policy>;
    using display_state_vector_t = folly::small_vector<display_state_t, 8, policy>;

    static_assert(sizeof(segment_vector_t) == 22);
    static_assert(sizeof(display_state_vector_t) == 10);

    segment_vector_t segments_ {};
    display_state_vector_t display_states_ {};

    index_t output_count_ {0};
    // TODO do we need input position?
    point_t input_position_ {};
    bool has_input_ {false};
};

static_assert(sizeof(SegmentTree) == 40);  // 22 + 10 + 2 + 4 + 1 (+ 1)

auto swap(SegmentTree &a, SegmentTree &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::SegmentTree &a, logicsim::SegmentTree &b) noexcept -> void;

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::SegmentPointType> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::SegmentPointType &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

#endif
