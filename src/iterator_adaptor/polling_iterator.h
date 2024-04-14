#ifndef LOGICSIM_ITERATOR_ADAPTOR_POLLING_ITERATOR_H
#define LOGICSIM_ITERATOR_ADAPTOR_POLLING_ITERATOR_H

#include <cassert>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace logicsim {

enum class polling_status {
    iterate,
    stop,
};

namespace detail {

struct polling_sentinel {};

template <typename T, typename State>
class polling_iterator {
   public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;

    using value_type = std::remove_reference_t<T>;
    using reference = std::conditional_t<std::is_reference_v<T>, T, T &>;
    using pointer_type = value_type *;

    using state_type = std::remove_cvref_t<State>;
    using mutator = polling_status (*)(State &);
    using generator = T (*)(const State &);

    [[nodiscard]] polling_iterator() = default;

    [[nodiscard]] polling_iterator(mutator m, generator g, State s,
                                   polling_status start_status)
        : mutator_ {m}, generator_ {g}, state_ {std::move(s)}, status_ {start_status} {}

    [[nodiscard]] friend auto operator==(const polling_iterator &it,
                                         polling_sentinel /*unused*/) noexcept -> bool {
        return (it.mutator_ == nullptr) || (it.status_ == polling_status::stop);
    }

    [[nodiscard]] friend auto operator==(polling_sentinel s,
                                         const polling_iterator &it) noexcept -> bool {
        return it == s;
    }

    [[nodiscard]] friend auto operator==(const polling_iterator &,
                                         const polling_iterator &) -> bool = default;

    auto operator++() -> polling_iterator & {
        assert(status_ == polling_status::iterate);
        assert(mutator_);
        status_ = mutator_(state_);
        return *this;
    }

    [[nodiscard]] auto operator++(int) -> polling_iterator {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    [[nodiscard]] auto operator*() const noexcept -> T {
        assert(generator_);
        return generator_(state_);
    }

   private:
    mutator mutator_ {nullptr};
    generator generator_ {nullptr};

    state_type state_ {};
    polling_status status_ {polling_status::stop};
};

}  // namespace detail

constexpr static inline auto polling_view_end = detail::polling_sentinel {};

/**
 * @brief:
 *
 * Note that State needs to be comparable and default constructable
 */
template <typename T, typename State>
    requires std::regular<State>
class polling_view {
   public:
    using iterator = detail::polling_iterator<T, State>;
    using const_iterator = detail::polling_iterator<T, State>;

    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using pointer_type = typename iterator::pointer_type;

    using state_type = typename iterator::state_type;
    using mutator = typename iterator::mutator;
    using generator = typename iterator::generator;

    static_assert(std::input_iterator<iterator>);
    static_assert(std::forward_iterator<iterator>);

    [[nodiscard]] polling_view() = default;

    [[nodiscard]] polling_view(mutator m, generator g)
        : mutator_ {m}, generator_ {g}, state_ {}, start_status_ {polling_status::stop} {}

    [[nodiscard]] polling_view(mutator m, generator g, State s,
                               polling_status start_status)
        : mutator_ {m},
          generator_ {g},
          state_ {std::move(s)},
          start_status_ {start_status} {}

    [[nodiscard]] auto begin() const -> iterator {
        return iterator {mutator_, generator_, state_, start_status_};
    }

    [[nodiscard]] auto end() const -> detail::polling_sentinel {
        return detail::polling_sentinel {};
    }

   private:
    mutator mutator_ {nullptr};
    generator generator_ {nullptr};
    state_type state_ {};
    polling_status start_status_ {polling_status::stop};
};

}  // namespace logicsim

template <typename T, typename State>
inline constexpr bool std::ranges::enable_view<logicsim::polling_view<T, State>> = true;

template <typename T, typename State>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::polling_view<T, State>> = true;

#endif
