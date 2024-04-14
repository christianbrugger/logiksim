#ifndef LOGICSIM_ALLOCATED_SIZE_TRACKED_RESOURCE_H
#define LOGICSIM_ALLOCATED_SIZE_TRACKED_RESOURCE_H

#include <gsl/gsl>

#include <memory_resource>

namespace logicsim {

/**
 * @brief A pmr resource that tracks the allocated size in bytes.
 */
class tracked_resource : public std::pmr::memory_resource {
   public:
    tracked_resource();
    explicit tracked_resource(std::pmr::memory_resource* upstream);

    [[nodiscard]] auto allocated_size() const -> std::size_t;

   private:
    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override;
    auto do_deallocate(void* p, std::size_t bytes, std::size_t alignment)
        -> void override;
    [[nodiscard]] auto do_is_equal(const std::pmr::memory_resource& other) const noexcept
        -> bool override;

   private:
    gsl::not_null<std::pmr::memory_resource*> upstream_;

    std::size_t allocated_size_ {};
};

}  // namespace logicsim

#endif
