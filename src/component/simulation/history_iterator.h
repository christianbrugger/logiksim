#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_ITERATOR_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_ITERATOR_H

#include "component/simulation/history_view.h"  // TODO remove circular

#include <cstddef>  // std::size_t
#include <iterator>
#include <type_traits>

namespace logicsim {

namespace simulation {

struct history_entry_t;

class HistoryIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using value_type = history_entry_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type;
    using reference = value_type;

    [[nodiscard]] explicit HistoryIterator() = default;
    [[nodiscard]] explicit HistoryIterator(HistoryView view, std::size_t index) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    auto operator++() noexcept -> HistoryIterator &;
    auto operator++(int) noexcept -> HistoryIterator;

    [[nodiscard]] auto operator==(const HistoryIterator &right) const noexcept -> bool;
    [[nodiscard]] auto operator-(const HistoryIterator &right) const noexcept
        -> difference_type;

   private:
    HistoryView view_ {};
    // from 0 to history.size() + 1
    std::size_t index_ {};
};

static_assert(std::forward_iterator<HistoryIterator>);

}  // namespace simulation

}  // namespace logicsim

#endif
