#include "allocated_size/tracked_resource.h"

#include <cassert>

namespace logicsim {

tracked_resource::tracked_resource() : upstream_ {std::pmr::get_default_resource()} {}

tracked_resource::tracked_resource(std::pmr::memory_resource* upstream)
    : upstream_ {upstream} {
    assert(upstream);
}

auto tracked_resource::allocated_size() const -> std::size_t {
    return allocated_size_;
}

auto tracked_resource::do_allocate(std::size_t bytes, std::size_t alignment) -> void* {
    assert(upstream_);
    const auto res = upstream_->allocate(bytes, alignment);

    // increment after allocation, so we are exception safe
    allocated_size_ += bytes;

    return res;
}

auto tracked_resource::do_deallocate(void* p, std::size_t bytes,
                                     std::size_t alignment) -> void {
    upstream_->deallocate(p, bytes, alignment);

    // deallocate should never throw, still we increment afterwards
    assert(bytes <= allocated_size_);
    allocated_size_ -= bytes;
}

auto tracked_resource::do_is_equal(const std::pmr::memory_resource& other) const noexcept
    -> bool {
    return this == &other;
}

}  // namespace logicsim
