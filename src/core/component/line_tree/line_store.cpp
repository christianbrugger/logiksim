#include "core/component/line_tree/line_store.h"

#include "core/algorithm/contains.h"
#include "core/allocated_size/folly_small_vector.h"
#include "core/allocated_size/trait.h"
#include "core/format/container.h"
#include "core/geometry/line.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

namespace line_tree {
auto LineStore::size() const noexcept -> std::size_t {
    Expects(lines_.size() == start_lengths_.size());

    return lines_.size();
}

auto LineStore::empty() const noexcept -> bool {
    Expects(lines_.size() == start_lengths_.size());

    return lines_.empty();
}

auto LineStore::allocated_size() const -> std::size_t {
    return get_allocated_size(lines_) +          //
           get_allocated_size(start_lengths_) +  //
           get_allocated_size(leaf_indices_);
}

auto LineStore::reserve(std::size_t capacity) -> void {
    lines_.reserve(capacity);
    start_lengths_.reserve(capacity);
    leaf_indices_.reserve(capacity);
}

auto LineStore::shrink_to_fit() -> void {
    lines_.shrink_to_fit();
    start_lengths_.shrink_to_fit();
    leaf_indices_.shrink_to_fit();
}

auto LineStore::format() const -> std::string {
    return fmt::format("LineStore(lines: {}, start_lengths: {}, leaf_indices: {})",
                       lines_, start_lengths_, leaf_indices_);
}

auto logicsim::line_tree::LineStore::add_first_line(line_t new_line) -> line_index_t {
    Expects(lines_.size() == start_lengths_.size());
    if (!empty()) [[unlikely]] {
        throw std::runtime_error("can only add first line to empty line store");
    }

    lines_.push_back(new_line);
    start_lengths_.push_back(length_t {0});
    leaf_indices_.push_back(line_index_t {0});

    Ensures(lines_.size() == start_lengths_.size());
    Ensures(leaf_indices_.size() <= lines_.size());
    return line_index_t {0};
}

auto LineStore::add_line(line_t new_line, line_index_t previous_index) -> line_index_t {
    if (empty()) [[unlikely]] {
        throw std::runtime_error("cannot add line to empty line tree");
    }

    Expects(!leaf_indices_.empty());
    Expects(lines_.size() == start_lengths_.size());

    const auto previous_line = line(previous_index);
    const auto last_index = this->last_index();
    const auto new_index = get_next(last_index);

    if (new_line.p0 != previous_line.p1) [[unlikely]] {
        throw std::runtime_error("New line must connect to the old line");
    }
    if (previous_index != last_index && contains(leaf_indices_, previous_index))
        [[unlikely]] {
        // Note this is needed, so we can keep track of leaves.
        throw std::runtime_error(
            "Previous index cannot refer to a leaf. "
            "Lines need to be added in depth first order");
    }
    if (contains(lines_, new_line.p1, &line_t::p1)) [[unlikely]] {
        // Note this is needed for 'starts_new_subtree' to work.
        throw std::runtime_error("endpoint needs to be unique");
    }

    lines_.push_back(new_line);
    start_lengths_.push_back(end_length(previous_index));

    if (previous_index == last_index) {
        // convert / move leaf
        leaf_indices_.back() = new_index;
    } else {
        // create new leaf
        leaf_indices_.push_back(new_index);
    }

    Ensures(lines_.size() == start_lengths_.size());
    Ensures(leaf_indices_.size() <= lines_.size());
    return new_index;
}

auto LineStore::line(line_index_t index) const -> line_t {
    return lines_.at(index.value);
}

auto LineStore::start_length(line_index_t index) const -> length_t {
    return start_lengths_.at(index.value);
}

auto LineStore::end_length(line_index_t index) const -> length_t {
    return start_length(index) + length_t {distance(line(index))};
}

auto LineStore::starts_new_subtree(line_index_t index) const -> bool {
    if (index == line_index_t {0}) {
        return false;
    }
    const auto previous = get_previous(index);

    return line(previous).p1 != line(index).p0;
}

auto LineStore::lines() const -> const line_vector_t& {
    return lines_;
}

auto LineStore::start_lengths() const -> const length_vector_t& {
    return start_lengths_;
}

auto LineStore::leaf_indices() const -> const index_vector_t& {
    return leaf_indices_;
}

auto LineStore::last_index() const -> line_index_t {
    Expects(!empty());
    return line_index_t {
        gsl::narrow_cast<line_index_t::value_type>(size() - std::size_t {1})};
}

}  // namespace line_tree

}  // namespace logicsim
