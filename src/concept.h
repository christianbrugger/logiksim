#ifndef LOGIKSIM_CONCEPT_H
#define LOGIKSIM_CONCEPT_H

namespace logicsim {

template <class T, class U>
concept explicitly_convertible_to = requires(T t) { static_cast<U>(t); };

}

template <typename R, typename V>
concept input_range_of = std::ranges::input_range<R> &&
                         std::convertible_to<std::ranges::range_reference_t<R>, V>;

#endif
