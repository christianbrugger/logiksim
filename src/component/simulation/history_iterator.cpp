#include "component/simulation/history_iterator.h"

namespace logicsim {

namespace simulation {

HistoryIterator::HistoryIterator(HistoryCalculationData data,
                                 history_index_t index) noexcept
    : data_ {std::move(data)}, index_ {index} {}

auto HistoryIterator::operator*() const -> value_type {
    return history_entry_t {{
        .first_time = get_time_extrapolated(data_, index_ - 1),
        .last_time = get_time_extrapolated(data_, index_),
        .value = get_value_extrapolated(data_, index_),
    }};
}

auto HistoryIterator::operator++() noexcept -> HistoryIterator & {
    ++index_;
    return *this;
}

auto HistoryIterator::operator++(int) noexcept -> HistoryIterator {
    const auto tmp = *this;
    ++(*this);
    return tmp;
}

auto HistoryIterator::operator==(const HistoryIterator &right) const noexcept -> bool {
    return index_ >= right.index_;
}

auto HistoryIterator::operator-(const HistoryIterator &right) const noexcept
    -> difference_type {
    return index_ - right.index_;
}

}  // namespace simulation

}  // namespace logicsim
