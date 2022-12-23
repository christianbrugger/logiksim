
#include <iterator>

namespace logicsim {

struct range {
    struct range_iterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = int;
        using pointer = value_type*;
        using reference = value_type&;
    };

    auto begin() const -> range_iterator { return range_iterator(); }

    auto end() const -> range_iterator { return range_iterator(); }
};

static_assert(std::forward_iterator<range::range_iterator>);

}  // namespace logicsim
