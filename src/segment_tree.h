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
    [[nodiscard]] explicit SegmentTree(segment_info_t segment);

    auto swap(SegmentTree &other) noexcept -> void;

    auto add_segment(segment_info_t segment) -> segment_index_t;
    auto add_tree(const SegmentTree &tree) -> void;

    auto update_segment(segment_index_t index, segment_info_t segment) -> void;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto segment_count() const noexcept -> std::size_t;
    [[nodiscard]] auto segment(std::size_t index) const -> segment_info_t;
    [[nodiscard]] auto segment(segment_index_t index) const -> segment_info_t;
    [[nodiscard]] auto segments() const -> std::span<const segment_info_t>;

    [[nodiscard]] auto has_input() const noexcept -> bool;
    [[nodiscard]] auto input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto input_position() const -> point_t;

    [[nodiscard]] auto output_count() const noexcept -> std::size_t;

    [[nodiscard]] auto format() const -> std::string;
    auto verify() const -> void;

   private:
    auto register_segment(segment_info_t segment) -> void;
    auto unregister_segment(segment_info_t segment) -> void;

   private:
    using policy = folly::small_vector_policy::policy_size_type<index_t>;
    using segment_vector_t = folly::small_vector<segment_info_t, 2, policy>;

    static_assert(sizeof(segment_vector_t) == 22);

    segment_vector_t segments_ {};

    index_t output_count_ {0};
    // TODO do we need input position?
    point_t input_position_ {};
    bool has_input_ {false};
};

static_assert(sizeof(SegmentTree) == 30);  // 22 + 2 + 4 + 1 (+ 1)

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
