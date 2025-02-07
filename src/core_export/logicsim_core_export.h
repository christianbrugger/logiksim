#ifndef LOGICSIM_CORE_LOGICSIM_EXPORT_H
#define LOGICSIM_CORE_LOGICSIM_EXPORT_H

#ifdef __cplusplus
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>  // unique_ptr
#include <stdexcept>
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

#ifdef __cplusplus
#define LS_NOEXCEPT noexcept
#else
#define LS_NOEXCEPT
#endif

//
// C DLL interface - hourclass pattern
//

#ifdef __cplusplus
extern "C" {
#endif
// NOLINTBEGIN(modernize-use-using)
// NOLINTBEGIN(modernize-use-trailing-return-type)

typedef struct ls_circuit_t ls_circuit_t;

LS_CORE_API ls_circuit_t* ls_circuit_construct() LS_NOEXCEPT;
LS_CORE_API void ls_circuit_destruct(ls_circuit_t* obj) LS_NOEXCEPT;
LS_CORE_API void ls_circuit_load(ls_circuit_t* obj, int32_t example_circuit) LS_NOEXCEPT;

/**
 * @brief: Render the layout to the given buffer.
 *
 * Terminates, if either width or height is negative.
 */
LS_CORE_API void ls_circuit_render_layout(ls_circuit_t* obj, int32_t width,
                                          int32_t height, double pixel_ratio,
                                          void* pixel_data, intptr_t stride) LS_NOEXCEPT;

typedef struct ls_point_device_fine_t {
    double x;
    double y;

#ifdef __cplusplus
    auto operator==(const ls_point_device_fine_t&) const -> bool = default;
#endif
} ls_point_device_fine_t;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t keyboard_modifiers;
    int32_t button;
    int32_t double_click;
} ls_mouse_press_event_t;

LS_CORE_API void ls_circuit_mouse_press(ls_circuit_t* obj,
                                        const ls_mouse_press_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t buttons;
} ls_mouse_move_event_t;

LS_CORE_API void ls_circuit_mouse_move(ls_circuit_t* obj,
                                       const ls_mouse_move_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    int32_t button;
} ls_mouse_release_event_t;

LS_CORE_API void ls_circuit_mouse_release(
    ls_circuit_t* obj, const ls_mouse_release_event_t* event) LS_NOEXCEPT;

// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-use-using)
#ifdef __cplusplus
}
#endif

//
// C++ abstraction - header only
//

#ifdef __cplusplus

namespace logicsim::exporting {

namespace detail {

inline auto ls_expects(auto&& value) -> void {
    if (!bool {value}) {
        std::terminate();
    }
}

// C++23 backport
template <class Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

}  // namespace detail

enum class ExampleCircuitType : uint8_t {
    example_circuit_1 = 1,
    example_circuit_2 = 2,
    example_circuit_3 = 3,
    example_circuit_4 = 4,
};

enum class MouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
};

constexpr inline auto all_mouse_buttons = std::array {
    MouseButton::Left,
    MouseButton::Right,
    MouseButton::Middle,
};

enum class KeyboardModifier : uint8_t {
    Shift = 0,
    Control = 1,
    Alt = 2,
};

constexpr inline auto all_keyboard_modifiers = std::array {
    KeyboardModifier::Shift,
    KeyboardModifier::Control,
    KeyboardModifier::Alt,
};

class MouseButtons {
   public:
    explicit MouseButtons() = default;

    explicit MouseButtons(uint32_t value) : value_ {value} {}

    auto set(MouseButton button, bool value = true) -> MouseButtons& {
        value_.set(detail::to_underlying(button), value);
        return *this;
    }

    [[nodiscard]] auto is_set(MouseButton button) const -> bool {
        return value_.test(detail::to_underlying(button));
    }

    [[nodiscard]] auto value() const -> uint32_t {
        const auto result = value_.to_ulong();

        if (result > std::numeric_limits<uint32_t>::max()) {
            throw std::overflow_error {"Value out of range"};
        }

        return static_cast<uint32_t>(result);
    }

    [[nodiscard]] auto operator==(const MouseButtons&) const -> bool = default;

   private:
    std::bitset<all_mouse_buttons.size()> value_;
};

class KeyboardModifiers {
   public:
    explicit KeyboardModifiers() = default;

    explicit KeyboardModifiers(uint32_t value) : value_ {value} {}

    auto set(KeyboardModifier modifier, bool value = true) -> KeyboardModifiers& {
        value_.set(detail::to_underlying(modifier), value);
        return *this;
    }

    [[nodiscard]] auto is_set(KeyboardModifier modifier) const -> bool {
        return value_.test(detail::to_underlying(modifier));
    }

    [[nodiscard]] auto value() const -> uint32_t {
        const auto result = value_.to_ulong();

        if (result > std::numeric_limits<uint32_t>::max()) {
            throw std::overflow_error {"Value out of range"};
        }

        return static_cast<uint32_t>(result);
    }

    [[nodiscard]] auto operator==(const KeyboardModifiers&) const -> bool = default;

   private:
    std::bitset<all_keyboard_modifiers.size()> value_;
};

struct MousePressEvent {
    ls_point_device_fine_t position {};
    KeyboardModifiers modifiers {};
    MouseButton button {MouseButton::Left};
    bool double_click {false};

    [[nodiscard]] auto operator==(const MousePressEvent&) const -> bool = default;
};

struct MouseMoveEvent {
    ls_point_device_fine_t position {};
    MouseButtons buttons {};

    [[nodiscard]] auto operator==(const MouseMoveEvent&) const -> bool = default;
};

struct MouseReleaseEvent {
    ls_point_device_fine_t position {};
    MouseButton button {MouseButton::Left};

    [[nodiscard]] auto operator==(const MouseReleaseEvent&) const -> bool = default;
};

namespace detail {

struct LSCircuitDeleter {
    auto operator()(ls_circuit_t* obj) noexcept -> void {
        ls_circuit_destruct(obj);
    };
};

}  // namespace detail

class CircuitInterface {
   public:
    inline auto load(ExampleCircuitType type) -> void;

    inline auto render_layout(int32_t width, int32_t height, double pixel_ratio,
                              void* pixel_data, intptr_t stride) -> void;

    inline auto mouse_press(const MousePressEvent& event) -> void;
    inline auto mouse_move(const MouseMoveEvent& event) -> void;
    inline auto mouse_release(const MouseReleaseEvent& event) -> void;

   private:
    std::unique_ptr<ls_circuit_t, detail::LSCircuitDeleter> obj_ {ls_circuit_construct()};
};

//
// C++ abstraction - Implementation
//

auto CircuitInterface::load(ExampleCircuitType type) -> void {
    detail::ls_expects(obj_);
    ls_circuit_load(obj_.get(), static_cast<int32_t>(type));
};

auto CircuitInterface::render_layout(int32_t width, int32_t height, double pixel_ratio,
                                     void* pixel_data, intptr_t stride) -> void {
    detail::ls_expects(obj_);
    ls_circuit_render_layout(obj_.get(), width, height, pixel_ratio, pixel_data, stride);
}

auto CircuitInterface::mouse_press(const MousePressEvent& event) -> void {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_press_event_t {
        .position = event.position,
        .keyboard_modifiers = event.modifiers.value(),
        .button = detail::to_underlying(event.button),
        .double_click = static_cast<int32_t>(event.double_click),
    };
    ls_circuit_mouse_press(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_move(const MouseMoveEvent& event) -> void {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_move_event_t {
        .position = event.position,
        .buttons = event.buttons.value(),
    };
    ls_circuit_mouse_move(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_release(const MouseReleaseEvent& event) -> void {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_release_event_t {
        .position = event.position,
        .button = detail::to_underlying(event.button),
    };
    ls_circuit_mouse_release(obj_.get(), &event_c);
};

}  // namespace logicsim::exporting

#endif

#endif
