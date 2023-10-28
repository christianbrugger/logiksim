#include "component/line_tree/line_store.h"

#include "algorithm/contains.h"
#include "geometry/line.h"
#include "geometry/orientation.h"

#include <gsl/gsl>

#include <stdexcept>

namespace logicsim {

namespace line_tree {
auto LineStore::size() const -> std::size_t {
    Expects(lines_.size() == start_lengths_.size());

    return lines_.size();
}

auto LineStore::empty() const -> std::size_t {
    Expects(lines_.size() == start_lengths_.size());

    return lines_.empty();
}

auto LineStore::reserve(std::size_t capacity) -> void {
    lines_.reserve(capacity);
    start_lengths_.reserve(capacity);
    leaf_lines_.reserve(capacity);
}

auto LineStore::shrink_to_fit() -> void {
    lines_.shrink_to_fit();
    start_lengths_.shrink_to_fit();
    leaf_lines_.shrink_to_fit();
}

auto logicsim::line_tree::LineStore::add_first_line(line_t new_line) -> line_index_t {
    Expects(lines_.size() == start_lengths_.size());
    if (!empty()) [[unlikely]] {
        throw std::runtime_error("can only add first line to empty line store");
    }

    lines_.push_back(new_line);
    start_lengths_.push_back(length_t {0});
    leaf_lines_.push_back(line_index_t {0});

    Ensures(lines_.size() == start_lengths_.size());
    Ensures(leaf_lines_.size() <= lines_.size());
    return line_index_t {0};
}

auto LineStore::add_line(line_t new_line, line_index_t previous_index) -> line_index_t {
    if (empty()) [[unlikely]] {
        throw std::runtime_error("cannot add line to empty line tree");
    }

    Expects(!leaf_lines_.empty());
    Expects(lines_.size() == start_lengths_.size());
    // TODO check colliding precondition

    const auto previous_line = line(previous_index);
    const auto last_index = this->last_index();
    const auto new_index = get_next(last_index);

    if (new_line.p0 != previous_line.p1) [[unlikely]] {
        throw std::runtime_error("New line must connect to the old line");
    }
    if ((previous_index == last_index) &&
        (is_horizontal(new_line) != is_horizontal(previous_line))) [[unlikely]] {
        throw std::runtime_error("Requires different orientation than old line");
    }
    if (previous_index != last_index && contains(leaf_lines_, previous_index))
        [[unlikely]] {
        throw std::runtime_error(
            "Previous index cannot refer to a leaf. "
            "Lines need to be added in depth first order");
    }

    lines_.push_back(new_line);
    start_lengths_.push_back(end_length(previous_index));

    if (previous_index == last_index) {
        leaf_lines_.back() = new_index;
    } else {
        leaf_lines_.push_back(new_index);
    }

    Ensures(lines_.size() == start_lengths_.size());
    Ensures(leaf_lines_.size() <= lines_.size());
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

auto LineStore::last_index() const -> line_index_t {
    Expects(!empty());
    return line_index_t {
        gsl::narrow_cast<line_index_t::value_type>(size() - std::size_t {1})};
}

}  // namespace line_tree

}  // namespace logicsim
