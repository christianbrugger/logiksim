#ifndef LOGICSIM_TEST_CONTAINER_VALUE_POINTER_PIMPL_H
#define LOGICSIM_TEST_CONTAINER_VALUE_POINTER_PIMPL_H

#include "container/value_pointer.h"

namespace logicsim {

namespace value_pointer_test {

struct incomplete_type;

}

extern template class value_pointer<value_pointer_test::incomplete_type>;

namespace value_pointer_test {

class IncompleteTypeTest {
   public:
    IncompleteTypeTest() = default;
    IncompleteTypeTest(int value);

    [[nodiscard]] auto value() const -> int;

   private:
    value_pointer<incomplete_type> value_;
};

}  // namespace value_pointer_test

}  // namespace logicsim

#endif
