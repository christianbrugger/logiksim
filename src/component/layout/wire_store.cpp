#include "component/layout/wire_store.h"

#include "allocated_size/std_vector.h"
#include "line_tree_generation.h"
#include "vocabulary/rect.h"
#include "vocabulary/wire_id.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

namespace layout {

namespace {

/**
 * @brief: Bounding rect value for empty wires.
 *
 * Note it is defined such it is outside the usual view space
 */
constexpr inline auto empty_bounding_rect =
    rect_t {point_t {-10'000, -10'000}, point_t {-10'000, -10'000}};

/**
 * @brief: The value of the bounding rect, when it is not computed yet.
 */
constexpr inline auto invalid_bounding_rect = rect_t {point_t {0, 0}, point_t {0, 0}};

}  // namespace

WireStore::WireStore()
    : segment_trees_(first_inserted_wire_id.value, SegmentTree {}),
      line_trees_(first_inserted_wire_id.value, LineTree {}),
      bounding_rects_(first_inserted_wire_id.value, invalid_bounding_rect) {}

auto WireStore::size() const -> std::size_t {
    Expects(segment_trees_.size() >= std::size_t {first_inserted_wire_id});

    static_assert(first_inserted_wire_id == wire_id_t {2});
    static_assert(colliding_wire_id == wire_id_t {1});
    static_assert(temporary_wire_id == wire_id_t {0});

    if (segment_trees_.size() > std::size_t {first_inserted_wire_id}) {
        return segment_trees_.size();
    }

    else if (!segment_trees_[colliding_wire_id.value].empty()) {
        return std::size_t {colliding_wire_id} + std::size_t {1};
    }

    else if (!segment_trees_[temporary_wire_id.value].empty()) {
        return std::size_t {temporary_wire_id} + std::size_t {1};
    }

    return std::size_t {0};
}

auto WireStore::empty() const -> bool {
    Expects(segment_trees_.size() >= std::size_t {first_inserted_wire_id});

    static_assert(first_inserted_wire_id == wire_id_t {2});
    static_assert(colliding_wire_id == wire_id_t {1});
    static_assert(temporary_wire_id == wire_id_t {0});

    return segment_trees_.size() == std::size_t {first_inserted_wire_id} &&
           segment_trees_[colliding_wire_id.value].empty() &&
           segment_trees_[temporary_wire_id.value].empty();
}

auto WireStore::allocated_size() const -> std::size_t {
    return get_allocated_size(segment_trees_) +  //
           get_allocated_size(line_trees_) +     //
           get_allocated_size(bounding_rects_);
}

auto WireStore::normalize() -> void {
    // clear caches
    std::ranges::fill(line_trees_, LineTree {});
    std::ranges::fill(bounding_rects_, invalid_bounding_rect);

    // normalize trees
    for (auto &tree : segment_trees_) {
        tree.normalize();
    }
}

auto WireStore::operator==(const WireStore &other) const -> bool {
    // caches are not part of our value
    return segment_trees_ == other.segment_trees_;
}

auto WireStore::add_wire() -> wire_id_t {
    if (segment_trees_.size() >= std::size_t {wire_id_t::max()} - std::size_t {1})
        [[unlikely]] {
        throw std::runtime_error("Reached maximum number of wire items.");
    }

    segment_trees_.emplace_back();
    line_trees_.emplace_back();
    bounding_rects_.emplace_back(empty_bounding_rect);

    return last_wire_id();
}

auto WireStore::swap_and_delete(wire_id_t wire_id) -> wire_id_t {
    const auto last_id = last_wire_id();

    swap(wire_id, last_id);
    delete_last();

    return last_id;
}

auto WireStore::swap(wire_id_t wire_id_1, wire_id_t wire_id_2) -> void {
    if (wire_id_1 < first_inserted_wire_id || wire_id_2 < first_inserted_wire_id)
        [[unlikely]] {
        throw std::runtime_error("can only swap inserted wires");
    }
    if (wire_id_1 == wire_id_2) {
        return;
    }

    const auto swap_ids = [&](auto &container) {
        using std::swap;
        swap(container.at(wire_id_1.value), container.at(wire_id_2.value));
    };

    swap_ids(segment_trees_);
    swap_ids(line_trees_);
    swap_ids(bounding_rects_);
}

auto WireStore::segment_tree(wire_id_t wire_id) const -> const SegmentTree & {
    return segment_trees_.at(wire_id.value);
}

auto WireStore::modifyable_segment_tree(wire_id_t wire_id) -> SegmentTree & {
    // reset caches
    if (wire_id >= first_inserted_wire_id) {
        line_trees_.at(wire_id.value) = LineTree {};
        bounding_rects_.at(wire_id.value) = invalid_bounding_rect;
    }

    return segment_trees_.at(wire_id.value);
}

auto WireStore::line_tree(wire_id_t wire_id) const -> const LineTree & {
    if (wire_id < first_inserted_wire_id) [[unlikely]] {
        throw std::runtime_error("only inserted wires have a line_tree");
    }
    auto &line_tree = line_trees_.at(wire_id.value);

    if (line_tree.empty()) {
        // update line tree
        const auto &segment_tree = this->segment_tree(wire_id);

        if (segment_tree.has_input()) {
            line_tree = generate_line_tree(segment_tree);
        }
    }

    return line_tree;
}

auto WireStore::bounding_rect(wire_id_t wire_id) const -> rect_t {
    if (!is_inserted(wire_id)) [[unlikely]] {
        throw std::runtime_error("only inserted wires have a stable bounding rect");
    }
    auto &rect = bounding_rects_.at(wire_id.value);

    if (rect == invalid_bounding_rect) {
        // update bounding rect
        const auto &segment_tree = this->segment_tree(wire_id);

        if (segment_tree.empty()) {
            rect = empty_bounding_rect;
        } else {
            rect = calculate_bounding_rect(segment_tree);
        }
    }

    return rect;
}

auto WireStore::delete_last() -> void {
    if (segment_trees_.size() <= std::size_t {first_inserted_wire_id}) [[unlikely]] {
        throw std::runtime_error("Only inserted wires can be deleted.");
    }

    segment_trees_.pop_back();
    line_trees_.pop_back();
    bounding_rects_.pop_back();
}

auto WireStore::last_wire_id() -> wire_id_t {
    return wire_id_t {
        gsl::narrow_cast<wire_id_t::value_type>(segment_trees_.size() - std::size_t {1})};
}

}  // namespace layout

}  // namespace logicsim
