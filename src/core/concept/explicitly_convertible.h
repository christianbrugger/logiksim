#ifndef LOGICSIM_CONCEPT_EXPLICITLY_CONVERTIBLE_H
#define LOGICSIM_CONCEPT_EXPLICITLY_CONVERTIBLE_H

namespace logicsim {

template <class T, class U>
concept explicitly_convertible_to = requires(T t) { static_cast<U>(t); };

}  // namespace logicsim

#endif
