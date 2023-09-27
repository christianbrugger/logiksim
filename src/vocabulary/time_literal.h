#ifndef LOGICSIM_VOCABULARY_TIME_LITERAL_H
#define LOGICSIM_VOCABULARY_TIME_LITERAL_H

#include <chrono>

namespace logicsim {

#ifdef _MSC_VER
#pragma warning(push)
// warning: literal suffix identifiers are reserved
// false-positive: we are not defining our own literal
#pragma warning(disable : 4455)
#endif
using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace logicsim

#endif
