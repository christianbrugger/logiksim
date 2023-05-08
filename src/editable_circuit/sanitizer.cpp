
#include "editable_circuit/sanitizer.h"

#include "editable_circuit/caches/collision_cache.h"
#include "editable_circuit/selection.h"
#include "geometry.h"
#include "layout.h"
#include "range.h"

#include <folly/small_vector.h>

#include <queue>
#include <utility>

namespace logicsim {

template <>
auto format(SanitizeMode mode) -> std::string {
    switch (mode) {
        case SanitizeMode::shrink:
            return "shrink";
        case SanitizeMode::expand:
            return "expand";
    }
    throw_exception("unknown SanitizeMode");
}

namespace {

class CrossingCache {
   private:
    enum class State : uint8_t {
        undefined,
        value_true,
        value_false,
    };

   public:
    CrossingCache(const CollisionCache &cache) : collision_cache_ {cache} {}

   private:
    auto is_colliding(offset_t offset, point_t point) const -> bool {
        auto &state = data_.at(offset.value);

        if (state == State::undefined) {
            state = collision_cache_.is_wires_crossing(point) ? State::value_true
                                                              : State::value_false;
        }

        return state == State::value_true;
    }

   public:
    auto set_line(ordered_line_t full_line) -> void {
        data_.clear();
        data_.resize(distance(full_line) + 1, State::undefined);
        full_line_ = full_line;
    }

    auto is_colliding(point_t point) const -> bool {
        const auto offset = to_offset(full_line_, point);
        return is_colliding(offset, point);
    }

    auto is_colliding(offset_t offset) const -> bool {
        const auto point = to_point(full_line_, offset);
        return is_colliding(offset, point);
    }

    auto max_offset() const -> offset_t {
        return to_part(full_line_).end;
    }

   private:
    const CollisionCache &collision_cache_;
    mutable std::vector<State> data_ {};
    ordered_line_t full_line_;
};

using part_vector_t = Selection::part_vector_t;

auto is_colliding(part_t part, const CrossingCache &cache) -> bool {
    return cache.is_colliding(part.begin) || cache.is_colliding(part.end);
}

auto is_colliding(std::span<const part_t> parts, const CrossingCache &cache) -> bool {
    return std::ranges::any_of(
        parts, [&cache](part_t part) { return is_colliding(part, cache); });
}
enum class Adaptation : uint8_t {
    unchanging,
    expanding,
    shrinking,
};

}  // namespace

template <>
auto format(Adaptation value) -> std::string {
    switch (value) {
        using enum Adaptation;

        case unchanging:
            return "unchanging";
        case expanding:
            return "expanding";
        case shrinking:
            return "shrinking";
    }
    throw_exception("unknown Adaptation value");
}

namespace {

struct part_view {
    part_vector_t parts;
    offset_t max_offset;

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return 2 * parts.size();
    }

    [[nodiscard]] constexpr auto operator[](std::size_t index) -> offset_t & {
        return (index % 2 == 0) ? parts[index / 2].begin : parts[index / 2].end;
    }

    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const offset_t & {
        return (index % 2 == 0) ? parts[index / 2].begin : parts[index / 2].end;
    }

    [[nodiscard]] constexpr auto at(std::size_t index) -> offset_t & {
        return (index % 2 == 0) ? parts.at(index / 2).begin : parts.at(index / 2).end;
    }

    [[nodiscard]] constexpr auto at(std::size_t index) const -> const offset_t & {
        return (index % 2 == 0) ? parts.at(index / 2).begin : parts.at(index / 2).end;
    }

    [[nodiscard]] constexpr auto is_begin(std::size_t index) const -> bool {
        return index % 2 == 0;
    }

    [[nodiscard]] constexpr auto is_end(std::size_t index) const -> bool {
        return !is_begin(index);
    }

   private:
    [[nodiscard]] auto can_increase(std::size_t index) const -> bool {
        if (index + 1 > size()) [[unlikely]] {
            throw_exception("index out of bounds");
        }

        if (index + 1 == size()) {
            return operator[](index) < max_offset;
        }
        return operator[](index) < operator[](index + 1);
    }

    [[nodiscard]] auto can_decrease(std::size_t index) const -> bool {
        if (index >= size()) [[unlikely]] {
            throw_exception("index out of bounds");
        }

        if (index == 0) {
            return operator[](index) > offset_t {0};
        }
        return operator[](index) > at(index - 1);
    }

    auto increase(std::size_t index) -> void {}

    auto decrease(std::size_t index) -> void {}

   public:
    [[nodiscard]] auto can_expand(std::size_t index) const -> bool {
        if (is_end(index)) {
            return can_increase(index);
        }
        return can_decrease(index);
    }

    [[nodiscard]] auto can_shrink(std::size_t index) const -> bool {
        if (is_end(index)) {
            return can_decrease(index);
        }
        return can_increase(index);
    }

    auto expand(std::size_t index) -> void {
        auto &part = parts.at(index / 2);

        if (is_end(index)) {
            part = part_t {part.begin, part.end + offset_t {1}};
        } else {
            part = part_t {part.begin - offset_t {1}, part.end};
        }
    }

    auto shrink(std::size_t index) -> void {
        auto &part = parts.at(index / 2);

        if (is_end(index)) {
            part = part_t {part.begin, part.end - offset_t {1}};
        } else {
            part = part_t {part.begin + offset_t {1}, part.end};
        }
    }
};

using adaption_vector_t = folly::small_vector<Adaptation, 8, detail::selection::policy>;

auto initial_adaptation(const part_view &offsets, const CrossingCache &cache)
    -> adaption_vector_t {
    auto result = adaption_vector_t(offsets.size(), Adaptation::unchanging);

    for (const auto index : range(offsets.size())) {
        if (cache.is_colliding(offsets[index])) {
            result[index] = Adaptation::expanding;
        }
    }
    return result;
};

auto next_adaptation(std::span<Adaptation> adaptation) -> bool {
    // we are counting up the values
    //    unchanging  -> ignored
    //    expanding   -> 0
    //    shrinking   -> 1

    for (const auto i : range(adaptation.size())) {
        if (adaptation[i] == Adaptation::expanding) {
            // flip highest to 1
            adaptation[i] = Adaptation::shrinking;

            // reset all before to 0
            for (const auto j : range(i)) {
                if (adaptation[j] != Adaptation::unchanging) {
                    adaptation[j] = Adaptation::expanding;
                }
            }

            return true;
        }
    }
    return false;
}

struct Mutation {
    unsigned int cost;

    part_view offsets;
    adaption_vector_t adaptations;

    // to get the next mutation
    int index;

    [[nodiscard]] constexpr auto operator==(const Mutation &other) const noexcept {
        return cost == other.cost;
    }

    [[nodiscard]] constexpr auto operator<=>(const Mutation &other) const noexcept {
        return cost <=> other.cost;
    }

    [[nodiscard]] auto format() const -> std::string {
        return fmt::format("Mutation(cost={}, index={}, parts={}, adaptations={})", cost,
                           index, offsets.parts, adaptations);
    }

    [[nodiscard]] constexpr auto is_first() const -> bool {
        return index < 0;
    }
};

auto next_mutation(Mutation &mutation) -> bool {
    const auto revert_last_mutation = [&mutation]() {
        if (!mutation.is_first()) {
            if (mutation.adaptations[mutation.index] == Adaptation::expanding) {
                mutation.offsets.shrink(mutation.index);
            } else {
                mutation.offsets.expand(mutation.index);
            }
        }
    };

    const auto first_index = gsl::narrow<std::size_t>(mutation.index + 1);

    for (auto index : range(first_index, mutation.offsets.size())) {
        if (mutation.adaptations[index] == Adaptation::expanding
            && mutation.offsets.can_expand(index)) {
            revert_last_mutation();

            mutation.offsets.expand(index);
            mutation.index = gsl::narrow<int>(index);

            return true;
        }

        if (mutation.adaptations[index] == Adaptation::shrinking
            && mutation.offsets.can_shrink(index)) {
            revert_last_mutation();

            mutation.offsets.shrink(index);
            mutation.index = gsl::narrow<int>(index);

            return true;
        }
    }
    return false;
}

auto mutation_from_adaption(part_view offsets, adaption_vector_t adaptation) -> Mutation {
    auto mutation = Mutation {
        .cost = 1,
        .offsets = std::move(offsets),
        .adaptations = std::move(adaptation),
        .index = -1,
    };
    return mutation;
}

auto with_increased_cost(Mutation mutation) {
    ++mutation.cost;
    mutation.index = -1;

    return mutation;
}

auto find_best_sanitized_parts(std::span<const part_t> parts, const CrossingCache &cache)
    -> Selection::part_vector_t {
    auto offsets = part_view {
        .parts = Selection::part_vector_t(parts.begin(), parts.end()),
        .max_offset = cache.max_offset(),
    };
    std::ranges::sort(offsets.parts);

    auto queue = std::vector<Mutation> {};

    const auto queue_push = [&queue](Mutation &&mutation) {
        queue.push_back(mutation);
        std::ranges::push_heap(queue, std::ranges::greater {});
    };

    // first adaptation
    auto adaptation = initial_adaptation(offsets, cache);
    auto more_adaptations = true;

    // TODO remove next line !!!
    queue_push(mutation_from_adaption(offsets, adaptation));

    while (true) {
        if ((queue.empty() || queue.front().cost > 2) && more_adaptations) {
            // queue_push(mutation_from_adaption(offsets, adaptation));
            // more_adaptations = next_adaptation(adaptation);
        }
        if (queue.empty()) {
            break;
        }
        // get mutation with lowest cost
        auto &mutation = queue.front();
        bool found_next = next_mutation(mutation);

        if (found_next) {
            if (!is_colliding(mutation.offsets.parts, cache)) {
                print("FOUND RESULT", mutation);
                return std::move(mutation.offsets.parts);
            }

            print(mutation);
            queue_push(with_increased_cost(mutation));
        } else {
            std::ranges::pop_heap(queue, std::ranges::greater {});
            queue.pop_back();
        }
    };
    // the completely selected or unselected segment is always valid
    throw_exception("we should always find one");
}

auto find_lower(offset_t offset, const CrossingCache &cache, offset_t limit) -> offset_t {
    while (offset > limit) {
        if (!cache.is_colliding(--offset)) {
            return offset;
        }
    }
    return offset;
}

auto find_higher(offset_t offset, const CrossingCache &cache, offset_t limit)
    -> offset_t {
    while (offset < limit) {
        if (!cache.is_colliding(++offset)) {
            return offset;
        }
    }
    return offset;
}

auto find_best_sanitized_parts_fast(std::span<const part_t> parts,
                                    const CrossingCache &cache, const SanitizeMode mode)
    -> part_vector_t {
    const auto max_offset = cache.max_offset();

    auto new_parts = part_vector_t {};

    for (const auto &part : parts) {
        const auto begin_colliding = cache.is_colliding(part.begin);
        const auto end_colliding = cache.is_colliding(part.end);

        auto new_offsets = [mode, &cache, part, begin_colliding, end_colliding,
                            max_offset]() {
            if (mode == SanitizeMode::expand) {
                return std::pair<offset_t, offset_t> {
                    begin_colliding ? find_lower(part.begin, cache, offset_t {0})
                                    : part.begin,
                    end_colliding ? find_higher(part.end, cache, max_offset) : part.end,
                };
            }
            return std::pair<offset_t, offset_t> {
                begin_colliding ? find_higher(part.begin, cache, part.end) : part.begin,
                end_colliding ? find_lower(part.end, cache, part.begin) : part.end,
            };
        }();

        if (new_offsets.first < new_offsets.second) {
            if (cache.is_colliding(new_offsets.first)) {
                throw_exception("first collides");
            }
            if (cache.is_colliding(new_offsets.second)) {
                throw_exception("second collides");
            }

            new_parts.push_back(part_t {new_offsets.first, new_offsets.second});
        }
    }
    if (mode == SanitizeMode::expand) {
        sort_and_merge_parts(new_parts);
    }
    fmt::print("parts = {}, new_parts = {}\n", parts, new_parts);
    return new_parts;
}

}  // namespace

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionCache &cache, SanitizeMode mode) -> void {
    auto crossing_cache = CrossingCache {cache};

    std::vector<segment_t> to_delete {};

    for (const auto &entry : selection.selected_segments()) {
        const auto segment = entry.first;
        const auto full_line = get_line(layout, segment);

        crossing_cache.set_line(full_line);
        if (is_colliding(entry.second, crossing_cache)) {
            auto new_segments
                = find_best_sanitized_parts_fast(entry.second, crossing_cache, mode);
            if (new_segments.empty()) {
                to_delete.push_back(segment);
            } else {
                selection.set_selection(segment, std::move(new_segments));
            }
        }
    }

    for (auto &segment : to_delete) {
        selection.set_selection(segment, {});
    }
}

auto sanitize_selection_simple(Selection &selection, const Layout &layout,
                               const CollisionCache &cache) -> void {
    auto segments = std::vector<segment_part_t> {};

    for (const auto &entry : selection.selected_segments()) {
        const auto full_line = get_line(layout, entry.first);

        for (const auto part : entry.second) {
            const auto line = to_line(full_line, part);

            if (cache.is_wires_crossing(line.p0) || cache.is_wires_crossing(line.p1)) {
                segments.push_back(segment_part_t {entry.first, part});
            }
        }
    }

    for (const auto segment : segments) {
        selection.remove_segment(segment);
    }
}

}  // namespace logicsim