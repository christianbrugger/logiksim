#ifndef LOGICSIM_CONTAINER_ENUM_MAP_H
#define LOGICSIM_CONTAINER_ENUM_MAP_H

#include "core/algorithm/to_underlying.h"

#include <array>
#include <cassert>

namespace logicsim {

template <typename Enum, std::size_t MaxValue, typename Value>
class EnumMap {
   public:
    [[nodiscard]] auto at(Enum key) -> Value&;
    [[nodiscard]] auto at(Enum key) const -> const Value&;

   private:
    std::array<Value, MaxValue> storage_ {};
};

//
// Implementation
//

template <typename Enum, std::size_t MaxValue, typename Value>
auto EnumMap<Enum, MaxValue, Value>::at(Enum key) -> Value& {
    return storage_.at(to_underlying(key));
}

template <typename Enum, std::size_t MaxValue, typename Value>
auto EnumMap<Enum, MaxValue, Value>::at(Enum key) const -> const Value& {
    return storage_.at(to_underlying(key));
}

}  // namespace logicsim

#endif
