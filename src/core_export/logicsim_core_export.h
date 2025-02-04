#ifndef LOGICSIM_CORE_LOGICSIM_EXPORT_H
#define LOGICSIM_CORE_LOGICSIM_EXPORT_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef _WIN32

#if defined(LS_CORE_LIB_BUILD_SHARED)
#define LS_CORE_API __declspec(dllexport)
#elif defined(LS_CORE_LIB_BUILD_STATIC)
#define LS_CORE_API
#else
#define LS_CORE_API __declspec(dllimport)
#endif

#else  // !_Win32

#if defined(LS_CORE_LIB_BUILD_SHARED) && (defined(__GNUC__) || defined(__clang__))
#define LS_CORE_API __attribute__((visibility("default")))
#else
#define LS_CORE_API
#endif

#endif

//
// C DLL interface - hourclass pattern
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ls_circuit* ls_circuit_t;

ls_circuit_t LS_CORE_API ls_circuit_construct();
void LS_CORE_API ls_circuit_destruct(ls_circuit_t obj);
void LS_CORE_API ls_circuit_load(ls_circuit_t obj, int32_t example_circuit);

/**
 * @brief: Render the layout to the given buffer.
 *
 * Terminates, if either width or height is negative.
 */
void LS_CORE_API ls_circuit_render_layout(ls_circuit_t obj, int32_t width, int32_t height,
                                          double pixel_ratio, void* pixel_data,
                                          intptr_t stride);

#ifdef __cplusplus
}
#endif

//
// C++ abstraction - header only
//

#ifdef __cplusplus

#include <memory>  // unique_ptr

namespace logicsim {
namespace core {

enum class ExampleCircuitType : int8_t {
    example_circuit_1 = 1,
    example_circuit_2 = 2,
    example_circuit_3 = 3,
    example_circuit_4 = 4,
};

namespace detail {

struct LSCircuitDeleter {
    auto operator()(ls_circuit_t obj) -> void {
        ls_circuit_destruct(obj);
    };
};

}  // namespace detail

class CircuitInterface {
   public:
    inline auto load(ExampleCircuitType type) -> void;
    inline auto render_layout(int32_t width, int32_t height, double pixel_ratio,
                              void* pixel_data, intptr_t stride) -> void;

   private:
    std::unique_ptr<ls_circuit, detail::LSCircuitDeleter> obj_ {ls_circuit_construct()};
};

//
// C++ abstraction - Implementation
//

inline auto ls_expects(auto&& value) -> void {
    if (!bool {value}) {
        std::terminate();
    }
}

auto CircuitInterface::load(ExampleCircuitType type) -> void {
    ls_expects(obj_);
    ls_circuit_load(obj_.get(), static_cast<int32_t>(type));
};

auto CircuitInterface::render_layout(int32_t width, int32_t height, double pixel_ratio,
                                     void* pixel_data, intptr_t stride) -> void {
    ls_expects(obj_);
    ls_circuit_render_layout(obj_.get(), width, height, pixel_ratio, pixel_data, stride);
}

}  // namespace core
}  // namespace logicsim

#endif

#endif
