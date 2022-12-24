#ifndef LOGIKSIM_CONCEPTS_H
#define LOGIKSIM_CONCEPTS_H

namespace logicsim {

template <class T, class U>
concept explicitly_convertible_to = requires(T t) { static_cast<U>(t); };

}

#endif