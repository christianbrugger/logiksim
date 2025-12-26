#ifndef LOGICSIM_CORE_LOGICSIM_EXPORT_H
#define LOGICSIM_CORE_LOGICSIM_EXPORT_H

#ifdef __cplusplus
#include <array>
#include <bitset>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>  // unique_ptr
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#else
#include <stdbool.h>
#include <stddef.h>
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

#ifdef __cplusplus
#define LS_NODISCARD [[nodiscard]]
#else
#define LS_NODISCARD
#endif

//
// C DLL interface - hourclass pattern
//

#ifdef __cplusplus
extern "C" {
#endif
// NOLINTBEGIN(modernize-use-using)
// NOLINTBEGIN(modernize-use-trailing-return-type)

typedef struct ls_optional_double_t {
    double value;
    bool is_valid;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_optional_double_t&) const -> bool = default;
#endif
} ls_optional_double_t;

// wraps std::string on logicsim side
// this is required so ownership can be transferred from logicsim to export
typedef struct ls_string_t ls_string_t;
LS_NODISCARD LS_CORE_API ls_string_t* ls_string_construct() LS_NOEXCEPT;
LS_CORE_API void ls_string_destruct(ls_string_t* obj) LS_NOEXCEPT;
LS_CORE_API const char* ls_string_data(const ls_string_t* obj) LS_NOEXCEPT;
LS_CORE_API size_t ls_string_size(const ls_string_t* obj) LS_NOEXCEPT;

// wraps std::filesystem::path on logicsim side
// this is required so ownership can be transferred from logicsim to export
#ifdef _WIN32
typedef wchar_t ls_path_char_t;
#else
typedef char ls_path_char_t;
#endif
typedef struct ls_path_t ls_path_t;
LS_NODISCARD LS_CORE_API ls_path_t* ls_path_construct() LS_NOEXCEPT;
LS_CORE_API void ls_path_destruct(ls_path_t* obj) LS_NOEXCEPT;
LS_CORE_API const ls_path_char_t* ls_path_data(const ls_path_t* obj) LS_NOEXCEPT;
LS_CORE_API size_t ls_path_size(const ls_path_t* obj) LS_NOEXCEPT;

// pass std::filesystem::path to logicsim side
// non owning view that does not transfer ownership
typedef struct ls_path_view_t {
    const ls_path_char_t* data;
    size_t size;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_path_view_t&) const -> bool = default;
#endif
} ls_path_view_t;

typedef struct ls_ui_status_t {
    bool repaint_required;
    bool config_changed;
    bool history_changed;
    bool dialogs_changed;
    bool filename_changed;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_ui_status_t&) const -> bool = default;
#endif
} ls_ui_status_t;

typedef struct ls_circuit_t ls_circuit_t;

// circuit::construct
LS_NODISCARD LS_CORE_API ls_circuit_t* ls_circuit_construct() LS_NOEXCEPT;

// circuit::destruct
LS_CORE_API void ls_circuit_destruct(ls_circuit_t* obj) LS_NOEXCEPT;

typedef struct ls_simulation_config_t {
    int64_t simulation_time_rate_ns;
    bool use_wire_delay;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_simulation_config_t&) const -> bool = default;
#endif
} ls_simulation_config_t;

typedef struct ls_render_config_t {
    uint8_t thread_count_enum;
    uint8_t wire_render_style_enum;
    //
    bool do_benchmark;
    bool show_circuit;
    bool show_collision_index;
    bool show_connection_index;
    bool show_selection_index;
    //
    bool show_render_borders;
    bool show_mouse_position;
    bool direct_rendering;
    bool jit_rendering;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_render_config_t&) const -> bool = default;
#endif
} ls_render_config_t;

typedef struct ls_circuit_state_t {
    uint8_t type_enum;
    uint8_t editing_default_mouse_action_enum;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_circuit_state_t&) const -> bool = default;
#endif
} ls_circuit_state_t;

typedef struct ls_ui_config_t {
    ls_simulation_config_t simulation;
    ls_render_config_t render;
    ls_circuit_state_t state;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_ui_config_t&) const -> bool = default;
#endif
} ls_ui_config_t;

// circuit::config
LS_NODISCARD LS_CORE_API ls_ui_config_t ls_circuit_config(const ls_circuit_t* obj)
    LS_NOEXCEPT;

LS_NODISCARD LS_CORE_API bool ls_circuit_is_render_do_benchmark(const ls_circuit_t* obj)
    LS_NOEXCEPT;

// circuit::set_config
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_set_config(ls_circuit_t* obj, const ls_ui_config_t* config) LS_NOEXCEPT;

typedef struct ls_ui_statistics_t {
    ls_optional_double_t simulation_events_per_second;
    double frames_per_second;
    double pixel_scale;
    int32_t image_width_px;
    int32_t image_height_px;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_ui_statistics_t&) const -> bool = default;
#endif
} ls_ui_statistics_t;

// circuit::statistic
LS_NODISCARD LS_CORE_API ls_ui_statistics_t ls_circuit_statistics(const ls_circuit_t* obj)
    LS_NOEXCEPT;

typedef struct ls_history_status_t {
    bool undo_available;
    bool redo_available;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_history_status_t&) const -> bool = default;
#endif
} ls_history_status_t;

// circuit::history_status
LS_NODISCARD LS_CORE_API ls_history_status_t
ls_circuit_history_status(const ls_circuit_t* obj) LS_NOEXCEPT;

// circuit::allocation_info
LS_CORE_API void ls_circuit_get_allocation_info(const ls_circuit_t* obj,
                                                ls_string_t* string) LS_NOEXCEPT;

typedef struct ls_point_device_fine_t {
    double x;
    double y;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_point_device_fine_t&) const -> bool = default;
#endif
} ls_point_device_fine_t;

// circuit::do_action
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_do_action(ls_circuit_t* obj, uint8_t action_enum,
                     const ls_point_device_fine_t* optional_position) LS_NOEXCEPT;

// circuit::file_action(FileAction action) -> FileActionResult;
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_file_action(ls_circuit_t* obj, uint8_t file_action_enum,
                       uint8_t* next_step_enum, ls_path_t* path_out) LS_NOEXCEPT;

typedef struct ls_modal_result_t {
    uint8_t modal_result_enum;
    ls_path_view_t path;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_modal_result_t&) const -> bool = default;
#endif
} ls_modal_result_t;

// circuit::submit_modal_result(const ModalResult& result) -> FileActionResult;
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_submit_modal_result(ls_circuit_t* obj, const ls_modal_result_t* modal_result,
                               uint8_t* next_step_enum, ls_path_t* path_out) LS_NOEXCEPT;

// TODO: remove
// circuit::load
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_load(ls_circuit_t* obj, uint8_t example_circuit_enum) LS_NOEXCEPT;

/**
 * @brief: Render the layout to the given buffer.
 *
 * Terminates, if either width or height is negative.
 */
LS_CORE_API void ls_circuit_render_layout(ls_circuit_t* obj, int32_t width,
                                          int32_t height, double pixel_ratio,
                                          void* pixel_data, intptr_t stride) LS_NOEXCEPT;

typedef struct ls_angle_delta_t {
    float horizontal_notches;
    float vertical_notches;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_angle_delta_t&) const -> bool = default;
#endif
} ls_angle_delta_t;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t keyboard_modifiers_bitset;
    uint8_t button_enum;
    bool double_click;
} ls_mouse_press_event_t;

// circuit::mouse_press
LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_mouse_press(
    ls_circuit_t* obj, const ls_mouse_press_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t buttons_bitset;
} ls_mouse_move_event_t;

// circuit::mouse_move
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_mouse_move(ls_circuit_t* obj, const ls_mouse_move_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    uint8_t button_enum;
} ls_mouse_release_event_t;

// circuit::mouse_release
LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_mouse_release(
    ls_circuit_t* obj, const ls_mouse_release_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    ls_angle_delta_t angle_delta;
    uint32_t keyboard_modifiers_bitset;
} ls_mouse_wheel_event_t;

// circuit::mouse_wheel
LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_mouse_wheel(
    ls_circuit_t* obj, const ls_mouse_wheel_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_mouse_wheel_event_t value;
    bool is_valid;
} ls_combine_wheel_event_result_t;

// combine_wheel_event
LS_NODISCARD LS_CORE_API ls_combine_wheel_event_result_t
ls_combine_wheel_event(const ls_mouse_wheel_event_t* first,
                       const ls_mouse_wheel_event_t* second) LS_NOEXCEPT;

LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_key_press(ls_circuit_t* obj, uint8_t key_enum) LS_NOEXCEPT;

// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-use-using)
#ifdef __cplusplus
}
#endif

//
// C++ abstraction - header only
//

#ifdef __cplusplus

[[nodiscard]] inline auto operator|(ls_ui_status_t a, ls_ui_status_t b)
    -> ls_ui_status_t {
    return ls_ui_status_t {
        .repaint_required = a.repaint_required || b.repaint_required,
        .config_changed = a.config_changed || b.config_changed,
        .history_changed = a.history_changed || b.history_changed,
        .dialogs_changed = a.dialogs_changed || b.dialogs_changed,
        .filename_changed = a.filename_changed || b.filename_changed,
    };
};

inline auto operator|=(ls_ui_status_t& a, ls_ui_status_t b) -> ls_ui_status_t& {
    a = a | b;
    return a;
};

namespace logicsim::exporting {

namespace detail {

inline auto ls_expects(auto&& value) -> void {
    if (!bool {value}) {
        std::terminate();
    }
}

// backport from c++23
template <class Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

struct LSStringDeleter {
    auto operator()(ls_string_t* obj) noexcept -> void {
        ls_string_destruct(obj);
    };
};

struct LSPathDeleter {
    auto operator()(ls_path_t* obj) noexcept -> void {
        ls_path_destruct(obj);
    };
};

}  // namespace detail

class WrappedString {
   public:
    [[nodiscard]] auto view() const -> std::string_view {
        return std::string_view {ls_string_data(get()), ls_string_size(get())};
    }

    [[nodiscard]] auto string() const -> std::string {
        return std::string {view()};
    }

    [[nodiscard]] auto get() -> ls_string_t* {
        detail::ls_expects(obj_);
        return obj_.get();
    }

    [[nodiscard]] auto get() const -> const ls_string_t* {
        detail::ls_expects(obj_);
        return obj_.get();
    }

   private:
    std::unique_ptr<ls_string_t, detail::LSStringDeleter> obj_ {ls_string_construct()};
};

class WrappedPath {
   public:
    using value_type = ls_path_char_t;
    using string_type = std::basic_string<value_type>;
    using string_view_type = std::basic_string_view<value_type>;

    [[nodiscard]] auto view() const -> string_view_type {
        return string_view_type {ls_path_data(get()), ls_path_size(get())};
    }

    [[nodiscard]] auto string() const -> string_type {
        return string_type {view()};
    }

    [[nodiscard]] auto path() const -> std::filesystem::path {
        return std::filesystem::path {string()};
    }

    [[nodiscard]] auto get() -> ls_path_t* {
        detail::ls_expects(obj_);
        return obj_.get();
    }

    [[nodiscard]] auto get() const -> const ls_path_t* {
        detail::ls_expects(obj_);
        return obj_.get();
    }

   private:
    std::unique_ptr<ls_path_t, detail::LSPathDeleter> obj_ {ls_path_construct()};
};

enum class ThreadCount : uint8_t {
    synchronous = 0,
    two = 1,
    four = 2,
    eight = 3,
};

enum class WireRenderStyle : uint8_t {
    red = 0,
    bold = 1,
    bold_red = 2,
};

enum class CircuitStateType : uint8_t {
    NonInteractive = 0,
    Simulation = 1,
    Editing = 2,
};

enum class DefaultMouseAction : uint8_t {
    // other
    selection = 0,
    insert_wire = 1,

    // logic items
    insert_button = 2,
    insert_led = 3,
    insert_display_number = 4,
    insert_display_ascii = 5,

    insert_and_element = 6,
    insert_or_element = 7,
    insert_xor_element = 8,
    insert_nand_element = 9,
    insert_nor_element = 10,

    insert_buffer_element = 11,
    insert_inverter_element = 12,
    insert_flipflop_jk = 13,
    insert_latch_d = 14,
    insert_flipflop_d = 15,
    insert_flipflop_ms_d = 16,

    insert_clock_generator = 17,
    insert_shift_register = 18,

    // decorations
    insert_decoration_text_element = 19,
};

struct time_rate_t {
    using rep = int64_t;
    using period = std::nano;
    using value_type = std::chrono::duration<rep, period>;

    value_type rate_per_second;

    [[nodiscard]] auto operator==(const time_rate_t& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_rate_t& other) const = default;
};

struct SimulationConfig {
    time_rate_t simulation_time_rate;
    bool use_wire_delay;

    [[nodiscard]] auto operator==(const SimulationConfig& other) const -> bool = default;
};

struct WidgetRenderConfig {
    ThreadCount thread_count;
    WireRenderStyle wire_render_style;

    bool do_benchmark;
    bool show_circuit;
    bool show_collision_index;
    bool show_connection_index;
    bool show_selection_index;

    bool show_render_borders;
    bool show_mouse_position;
    bool direct_rendering;
    bool jit_rendering;

    [[nodiscard]] auto operator==(const WidgetRenderConfig&) const -> bool = default;
};

struct CircuitWidgetState {
    CircuitStateType type;
    DefaultMouseAction editing_default_mouse_action;

    [[nodiscard]] auto operator==(const CircuitWidgetState&) const -> bool = default;
};

struct CircuitUIConfig {
    SimulationConfig simulation;
    WidgetRenderConfig render;
    CircuitWidgetState state;

    [[nodiscard]] auto operator==(const CircuitUIConfig&) const -> bool = default;
};

enum class UserAction : uint8_t {
    clear_circuit = 0,
    reload_circuit = 1,

    undo = 2,
    redo = 3,
    select_all = 4,
    copy_selected = 5,
    paste_from_clipboard = 6,
    cut_selected = 7,
    delete_selected = 8,

    zoom_in = 9,
    zoom_out = 10,
    reset_view = 11,
};

enum class FileAction : uint8_t {
    new_file = 0,
    open_file = 1,
    save_file = 2,
    save_as_file = 3,

    load_example_simple = 4,
    load_example_elements_wires = 5,
    load_example_elements = 6,
    load_example_wires = 7,
};

namespace detail {

// std::optional<NextActionStep> - variant
enum class NextStepEnum : uint8_t {
    // nullopt
    no_next_step = 0,

    // ModalRequest - variant
    save_current_modal = 1,
    open_file_modal = 2,
    save_file_modal = 3,

    // ErrorMessage - variant
    save_file_error = 4,
    open_file_error = 5,
};

// ModalResult - variant
enum class ModalResultEnum : uint8_t {
    save_current_yes = 0,
    save_current_no = 1,
    save_current_cancel = 3,

    open_file_open = 4,
    open_file_cancel = 5,

    save_file_save = 6,
    save_file_cancel = 7
};

}  // namespace detail

///////////////////////////////////////////

struct SaveCurrentModal {
    std::filesystem::path filename;
    [[nodiscard]] auto operator==(const SaveCurrentModal&) const -> bool = default;
};

struct OpenFileModal {
    [[nodiscard]] auto operator==(const OpenFileModal&) const -> bool = default;
};

struct SaveFileModal {
    [[nodiscard]] auto operator==(const SaveFileModal&) const -> bool = default;
};

using ModalRequest = std::variant<SaveCurrentModal, OpenFileModal, SaveFileModal>;

static_assert(std::regular<ModalRequest>);

struct SaveCurrentYes {
    [[nodiscard]] auto operator==(const SaveCurrentYes&) const -> bool = default;
};

struct SaveCurrentNo {
    [[nodiscard]] auto operator==(const SaveCurrentNo&) const -> bool = default;
};

struct SaveCurrentCancel {
    [[nodiscard]] auto operator==(const SaveCurrentCancel&) const -> bool = default;
};

struct OpenFileOpen {
    std::filesystem::path filename;
    [[nodiscard]] auto operator==(const OpenFileOpen&) const -> bool = default;
};

struct OpenFileCancel {
    [[nodiscard]] auto operator==(const OpenFileCancel&) const -> bool = default;
};

struct SaveFileSave {
    std::filesystem::path filename;
    [[nodiscard]] auto operator==(const SaveFileSave&) const -> bool = default;
};

struct SaveFileCancel {
    [[nodiscard]] auto operator==(const SaveFileCancel&) const -> bool = default;
};

using ModalResult = std::variant<SaveCurrentYes, SaveCurrentNo, SaveCurrentCancel,  //
                                 OpenFileOpen, OpenFileCancel,                      //
                                 SaveFileSave, SaveFileCancel>;

static_assert(std::regular<ModalResult>);

struct SaveFileError {
    std::filesystem::path filename;
    [[nodiscard]] auto operator==(const SaveFileError&) const -> bool = default;
};

struct OpenFileError {
    std::filesystem::path filename;
    [[nodiscard]] auto operator==(const OpenFileError&) const -> bool = default;
};

using ErrorMessage = std::variant<SaveFileError, OpenFileError>;

using NextActionStep = std::variant<ErrorMessage, ModalRequest>;

struct FileActionResult {
    ls_ui_status_t status;
    std::optional<NextActionStep> next_step;
    [[nodiscard]] auto operator==(const FileActionResult&) const -> bool = default;
};

static_assert(std::regular<FileActionResult>);

///////////////////////////////////////////

// TODO: delete
enum class ExampleCircuitType : uint8_t {
    simple = 1,
    elements_wires = 2,
    elements = 3,
    wires = 4,
};

enum class VirtualKey : uint8_t {
    Enter = 0,
    Escape = 1,
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
    MouseButtons() = default;

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

    [[nodiscard]] explicit operator bool() const {
        return value_.any();
    }

   private:
    std::bitset<all_mouse_buttons.size()> value_;
};

class KeyboardModifiers {
   public:
    KeyboardModifiers() = default;

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

    [[nodiscard]] explicit operator bool() const {
        return value_.any();
    }

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

struct MouseWheelEvent {
    ls_point_device_fine_t position {};
    ls_angle_delta_t angle_delta {};
    KeyboardModifiers modifiers {};

    [[nodiscard]] auto operator==(const MouseWheelEvent&) const -> bool = default;
};

struct UserActionEvent {
    UserAction action {UserAction::clear_circuit};
    std::optional<ls_point_device_fine_t> position {};

    [[nodiscard]] auto operator==(const UserActionEvent&) const -> bool = default;
};

[[nodiscard]] inline auto combine_wheel_event(const MouseWheelEvent& first,
                                              const MouseWheelEvent& second)
    -> std::optional<MouseWheelEvent>;

namespace detail {

struct LSCircuitDeleter {
    auto operator()(ls_circuit_t* obj) noexcept -> void {
        ls_circuit_destruct(obj);
    };
};

}  // namespace detail

class CircuitInterface {
   public:
    [[nodiscard]] inline auto set_config(const CircuitUIConfig& config) -> ls_ui_status_t;
    [[nodiscard]] inline auto config() const -> CircuitUIConfig;
    [[nodiscard]] inline auto is_render_do_benchmark() const -> bool;
    [[nodiscard]] inline auto statistics() const -> ls_ui_statistics_t;
    [[nodiscard]] inline auto history_status() const -> ls_history_status_t;
    [[nodiscard]] inline auto allocation_info() const -> std::string;

    [[nodiscard]] inline auto do_action(const UserActionEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto load(ExampleCircuitType type)
        -> ls_ui_status_t;  // TODO remove
    [[nodiscard]] inline auto file_action(FileAction action) -> FileActionResult;
    [[nodiscard]] inline auto submit_modal_result(const ModalResult& result)
        -> FileActionResult;

    inline auto render_layout(int32_t width, int32_t height, double pixel_ratio,
                              void* pixel_data, intptr_t stride) -> void;

    [[nodiscard]] inline auto mouse_press(const MousePressEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto mouse_move(const MouseMoveEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto mouse_release(const MouseReleaseEvent& event)
        -> ls_ui_status_t;
    [[nodiscard]] inline auto mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto key_press(VirtualKey key) -> ls_ui_status_t;

   private:
    [[nodiscard]] inline auto get() const -> const ls_circuit_t*;
    [[nodiscard]] inline auto get() -> ls_circuit_t*;

   private:
    std::unique_ptr<ls_circuit_t, detail::LSCircuitDeleter> obj_ {ls_circuit_construct()};
};

//
// C++ abstraction - Implementation
//

namespace detail {

[[nodiscard]] inline auto from_exp(const MouseWheelEvent& event)
    -> ls_mouse_wheel_event_t {
    return ls_mouse_wheel_event_t {
        .position = event.position,
        .angle_delta = event.angle_delta,
        .keyboard_modifiers_bitset = event.modifiers.value(),
    };
}

[[nodiscard]] inline auto to_exp(const ls_mouse_wheel_event_t& event) -> MouseWheelEvent {
    return MouseWheelEvent {
        .position = event.position,
        .angle_delta = event.angle_delta,
        .modifiers = KeyboardModifiers {event.keyboard_modifiers_bitset},
    };
}

[[nodiscard]] inline auto from_exp(const CircuitUIConfig& config) -> ls_ui_config_t {
    return ls_ui_config_t {
        .simulation =
            ls_simulation_config_t {
                .simulation_time_rate_ns =
                    config.simulation.simulation_time_rate.rate_per_second.count(),
                .use_wire_delay = config.simulation.use_wire_delay,
            },
        .render =
            ls_render_config_t {
                .thread_count_enum = to_underlying(config.render.thread_count),
                .wire_render_style_enum = to_underlying(config.render.wire_render_style),

                .do_benchmark = config.render.do_benchmark,
                .show_circuit = config.render.show_circuit,
                .show_collision_index = config.render.show_collision_index,
                .show_connection_index = config.render.show_connection_index,
                .show_selection_index = config.render.show_selection_index,

                .show_render_borders = config.render.show_render_borders,
                .show_mouse_position = config.render.show_mouse_position,
                .direct_rendering = config.render.direct_rendering,
                .jit_rendering = config.render.jit_rendering,
            },
        .state =
            ls_circuit_state_t {
                .type_enum = to_underlying(config.state.type),
                .editing_default_mouse_action_enum =
                    to_underlying(config.state.editing_default_mouse_action),
            },
    };
}

[[nodiscard]] inline auto to_exp(const ls_ui_config_t& config) -> CircuitUIConfig {
    return CircuitUIConfig {
        .simulation =
            SimulationConfig {
                .simulation_time_rate = time_rate_t {time_rate_t::value_type {
                    config.simulation.simulation_time_rate_ns}},
                .use_wire_delay = config.simulation.use_wire_delay,
            },
        .render =
            WidgetRenderConfig {
                .thread_count = static_cast<ThreadCount>(config.render.thread_count_enum),
                .wire_render_style =
                    static_cast<WireRenderStyle>(config.render.wire_render_style_enum),

                .do_benchmark = config.render.do_benchmark,
                .show_circuit = config.render.show_circuit,
                .show_collision_index = config.render.show_collision_index,
                .show_connection_index = config.render.show_connection_index,
                .show_selection_index = config.render.show_selection_index,

                .show_render_borders = config.render.show_render_borders,
                .show_mouse_position = config.render.show_mouse_position,
                .direct_rendering = config.render.direct_rendering,
                .jit_rendering = config.render.jit_rendering,
            },
        .state =
            CircuitWidgetState {
                .type = static_cast<CircuitStateType>(config.state.type_enum),
                .editing_default_mouse_action = static_cast<DefaultMouseAction>(
                    config.state.editing_default_mouse_action_enum),
            },
    };
}

[[nodiscard]] inline auto to_exp_next_step(uint8_t next_step_enum,
                                           const WrappedPath& path_out)
    -> std::optional<NextActionStep> {
    //
    switch (static_cast<NextStepEnum>(next_step_enum)) {
        using enum NextStepEnum;

        case no_next_step:
            ls_expects(path_out.view().empty());
            return std::nullopt;

        case save_current_modal:
            return SaveCurrentModal {
                .filename = path_out.path(),
            };
        case open_file_modal:
            ls_expects(path_out.view().empty());
            return OpenFileModal {};
        case save_file_modal:
            ls_expects(path_out.view().empty());
            return SaveFileModal {};

        case save_file_error:
            return SaveFileError {
                .filename = path_out.path(),
            };
        case open_file_error:
            return OpenFileError {
                .filename = path_out.path(),
            };
    };
    std::terminate();
}

[[nodiscard]] inline auto to_exp_file_action_result(ls_ui_status_t status,
                                                    uint8_t next_step_enum,
                                                    const WrappedPath& path_out)
    -> FileActionResult {
    return FileActionResult {
        .status = status,
        .next_step = to_exp_next_step(next_step_enum, path_out),
    };
}

[[nodiscard]] inline auto from_exp(const ModalResult& modal_result)
    -> std::pair<ModalResultEnum, std::filesystem::path> {
    if (std::holds_alternative<SaveCurrentYes>(modal_result)) {
        return {ModalResultEnum::save_current_yes, {}};
    }
    if (std::holds_alternative<SaveCurrentNo>(modal_result)) {
        return {ModalResultEnum::save_current_no, {}};
    }
    if (std::holds_alternative<SaveCurrentCancel>(modal_result)) {
        return {ModalResultEnum::save_current_cancel, {}};
    }

    if (const auto data = std::get_if<OpenFileOpen>(&modal_result)) {
        return {ModalResultEnum::open_file_open, data->filename};
    }
    if (std::holds_alternative<OpenFileCancel>(modal_result)) {
        return {ModalResultEnum::open_file_cancel, {}};
    }

    if (const auto data = std::get_if<OpenFileOpen>(&modal_result)) {
        return {ModalResultEnum::save_file_save, data->filename};
    }
    if (std::holds_alternative<SaveFileCancel>(modal_result)) {
        return {ModalResultEnum::save_file_cancel, {}};
    }
    std::terminate();
}

}  // namespace detail

auto combine_wheel_event(const MouseWheelEvent& first, const MouseWheelEvent& second)
    -> std::optional<MouseWheelEvent> {
    const auto first_c = detail::from_exp(first);
    const auto second_c = detail::from_exp(second);

    if (const auto result = ls_combine_wheel_event(&first_c, &second_c);
        result.is_valid) {
        return detail::to_exp(result.value);
    }
    return std::nullopt;
}

auto CircuitInterface::get() const -> const ls_circuit_t* {
    detail::ls_expects(obj_);
    return obj_.get();
}

auto CircuitInterface::get() -> ls_circuit_t* {
    detail::ls_expects(obj_);
    return obj_.get();
}

auto CircuitInterface::set_config(const CircuitUIConfig& config) -> ls_ui_status_t {
    const auto config_c = detail::from_exp(config);
    return ls_circuit_set_config(get(), &config_c);
}

auto CircuitInterface::config() const -> CircuitUIConfig {
    return detail::to_exp(ls_circuit_config(get()));
}

auto CircuitInterface::is_render_do_benchmark() const -> bool {
    return ls_circuit_is_render_do_benchmark(get());
}

auto CircuitInterface::statistics() const -> ls_ui_statistics_t {
    return ls_circuit_statistics(get());
}

auto CircuitInterface::history_status() const -> ls_history_status_t {
    return ls_circuit_history_status(get());
}

auto CircuitInterface::allocation_info() const -> std::string {
    auto data = WrappedString {};
    ls_circuit_get_allocation_info(get(), data.get());
    return data.string();
}

auto CircuitInterface::do_action(const UserActionEvent& event) -> ls_ui_status_t {
    return ls_circuit_do_action(get(), detail::to_underlying(event.action),
                                event.position ? &event.position.value() : nullptr);
}

auto CircuitInterface::load(ExampleCircuitType type) -> ls_ui_status_t {
    return ls_circuit_load(get(), detail::to_underlying(type));
};

auto CircuitInterface::file_action(FileAction action) -> FileActionResult {
    auto next_step_enum = uint8_t {};
    auto path_out = WrappedPath {};
    const auto status = ls_circuit_file_action(get(), detail::to_underlying(action),
                                               &next_step_enum, path_out.get());

    return detail::to_exp_file_action_result(status, next_step_enum, path_out);
}

auto CircuitInterface::submit_modal_result(const ModalResult& result)
    -> FileActionResult {
    const auto [modal_result_enum, path] = detail::from_exp(result);
    const auto modal_result = ls_modal_result_t {
        .modal_result_enum = detail::to_underlying(modal_result_enum),
        .path =
            ls_path_view_t {
                .data = path.native().data(),
                .size = path.native().size(),
            },
    };

    auto next_step_enum = uint8_t {};
    auto path_out = WrappedPath {};
    const auto status = ls_circuit_submit_modal_result(get(), &modal_result,
                                                       &next_step_enum, path_out.get());

    return detail::to_exp_file_action_result(status, next_step_enum, path_out);
}

auto CircuitInterface::render_layout(int32_t width, int32_t height, double pixel_ratio,
                                     void* pixel_data, intptr_t stride) -> void {
    ls_circuit_render_layout(get(), width, height, pixel_ratio, pixel_data, stride);
}

auto CircuitInterface::mouse_press(const MousePressEvent& event) -> ls_ui_status_t {
    const auto event_c = ls_mouse_press_event_t {
        .position = event.position,
        .keyboard_modifiers_bitset = event.modifiers.value(),
        .button_enum = detail::to_underlying(event.button),
        .double_click = event.double_click,
    };
    return ls_circuit_mouse_press(get(), &event_c);
};

auto CircuitInterface::mouse_move(const MouseMoveEvent& event) -> ls_ui_status_t {
    const auto event_c = ls_mouse_move_event_t {
        .position = event.position,
        .buttons_bitset = event.buttons.value(),
    };
    return ls_circuit_mouse_move(get(), &event_c);
};

auto CircuitInterface::mouse_release(const MouseReleaseEvent& event) -> ls_ui_status_t {
    const auto event_c = ls_mouse_release_event_t {
        .position = event.position,
        .button_enum = detail::to_underlying(event.button),
    };
    return ls_circuit_mouse_release(get(), &event_c);
};

auto CircuitInterface::mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status_t {
    const auto event_c = detail::from_exp(event);
    return ls_circuit_mouse_wheel(get(), &event_c);
};

auto CircuitInterface::key_press(VirtualKey key) -> ls_ui_status_t {
    return ls_circuit_key_press(get(), detail::to_underlying(key));
};

}  // namespace logicsim::exporting

#endif

#endif
