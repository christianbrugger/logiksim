#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_ITERATOR_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_ITERATOR_H

#include "core/component/simulation/history_calculation.h"
#include "core/component/simulation/history_entry.h"
#include "core/component/simulation/history_index.h"

#include <cstddef>  // std::size_t
#include <iterator>
#include <type_traits>

namespace logicsim {

namespace simulation {

/**
 * @brief: Iterator over History.
 */
class HistoryIterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    using value_type = history_entry_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type;
    using reference = value_type;

    [[nodiscard]] explicit HistoryIterator() = default;
    [[nodiscard]] explicit HistoryIterator(HistoryCalculationData data,
                                           history_index_t index) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    auto operator++() -> HistoryIterator &;
    auto operator++(int) -> HistoryIterator;

    [[nodiscard]] auto operator==(const HistoryIterator &right) const noexcept -> bool;
    [[nodiscard]] auto operator-(const HistoryIterator &right) const noexcept
        -> difference_type;

   private:
    HistoryCalculationData data_ {};
    history_index_t index_ {};
};

static_assert(std::forward_iterator<HistoryIterator>);

}  // namespace simulation

}  // namespace logicsim

#endif
