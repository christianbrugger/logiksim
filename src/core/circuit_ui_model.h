#ifndef LOGICSIM_CORE_CIRCUIT_UI_MODEL_H
#define LOGICSIM_CORE_CIRCUIT_UI_MODEL_H

#include "core/component/circuit_ui_model/circuit_renderer.h"
#include "core/component/circuit_ui_model/circuit_store.h"
#include "core/component/circuit_ui_model/dialog_manager.h"
#include "core/component/circuit_ui_model/mouse_logic/editing_logic_manager.h"
#include "core/component/circuit_ui_model/mouse_logic/mouse_drag_logic.h"
#include "core/vocabulary/circuit_ui_config.h"
#include "core/vocabulary/device_pixel_ratio.h"
#include "core/vocabulary/history_status.h"
#include "core/vocabulary/mouse_event.h"
#include "core/vocabulary/render_mode.h"
#include "core/vocabulary/ui_status.h"
#include "core/vocabulary/view_config.h"

#include <gsl/gsl>

#include <coroutine>
#include <filesystem>
#include <optional>

// TODO: use more device_pixel_ratio_t
// TODO: use enum for example int
// TODO: pass device pixel ratio directly to render methods
// TODO: also fix config in circuit-renderer (don't store size & ratio)
// TODO: use -Wconversion for clang & gcc
// TODO: instead of serialize use some history ID to check if circuit needs saving
// TODO: rename vocabulary circuit widget state (anything else that is widget)

namespace logicsim {

class SettingDialogManager;
struct selection_id_t;
struct CircuitWidgetAllocInfo;
class EditableCircuit;
class LoadError;

namespace circuit_ui_model {
/**
 * @brief: Statistics of the Circuit Widget
 */
struct Statistics {
    std::optional<double> simulation_events_per_second;
    double frames_per_second;
    double pixel_scale;
    BLSizeI image_size;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const Statistics&) const -> bool = default;
};

static_assert(std::regular<Statistics>);

/**
 * @brief: Any outside actions that does not require arguments or return values.
 */
enum class UserAction : uint8_t {
    /**
     * @brief: Reloads the circuit and frees memory. Mostly for debugging purposes.
     */
    reload_circuit,

    undo,
    redo,
    select_all,
    copy_selected,
    paste_from_clipboard,
    cut_selected,
    delete_selected,

    zoom_in,
    zoom_out,
    reset_view,
};

struct UnsavedName {
    std::filesystem::path name;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const UnsavedName&) const -> bool = default;
};

struct SavedPath {
    std::filesystem::path path;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SavedPath&) const -> bool = default;
};

using NameOrPath = std::variant<UnsavedName, SavedPath>;

[[nodiscard]] auto get_filename(const NameOrPath& name_or_path) -> std::filesystem::path;

struct SaveInformation {
    NameOrPath name_or_path {};
    std::optional<std::string> serialized {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveInformation&) const -> bool = default;
};

static_assert(std::regular<SaveInformation>);

enum class FileAction : uint8_t {
    new_file,
    open_file,
    save_file,
    save_as_file,

    load_example_simple,
    load_example_elements_and_wires,
    load_example_elements,
    load_example_wires,
};

struct SaveCurrentModal {
    std::filesystem::path filename;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveCurrentModal&) const -> bool = default;
};

struct OpenFileModal {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const OpenFileModal&) const -> bool = default;
};

struct SaveFileModal {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveFileModal&) const -> bool = default;
};

using ModalRequest = std::variant<SaveCurrentModal, OpenFileModal, SaveFileModal>;

static_assert(std::regular<ModalRequest>);

struct SaveCurrentYes {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveCurrentYes&) const -> bool = default;
};

struct SaveCurrentNo {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveCurrentNo&) const -> bool = default;
};

struct SaveCurrentCancel {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveCurrentCancel&) const -> bool = default;
};

struct OpenFileOpen {
    std::filesystem::path filename;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const OpenFileOpen&) const -> bool = default;
};

struct OpenFileCancel {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const OpenFileCancel&) const -> bool = default;
};

struct SaveFileSave {
    std::filesystem::path filename;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveFileSave&) const -> bool = default;
};

struct SaveFileCancel {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveFileCancel&) const -> bool = default;
};

using ModalResult = std::variant<SaveCurrentYes, SaveCurrentNo, SaveCurrentCancel,  //
                                 OpenFileOpen, OpenFileCancel,                      //
                                 SaveFileSave, SaveFileCancel>;

static_assert(std::regular<ModalResult>);

struct SaveFileError {
    std::filesystem::path filename;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SaveFileError&) const -> bool = default;
};

struct OpenFileError {
    std::filesystem::path filename;
    std::string message;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const OpenFileError&) const -> bool = default;
};

using ErrorMessage = std::variant<SaveFileError, OpenFileError>;

using NextActionStep = std::variant<ErrorMessage, ModalRequest>;

struct FileActionResult {
    UIStatus status;
    std::optional<NextActionStep> next_step;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const FileActionResult&) const -> bool = default;
};

static_assert(std::regular<FileActionResult>);

struct CircuitAction {
    FileAction action {};

    // only used for some actions (open_file, save_file, save_as_file)
    std::optional<std::filesystem::path> filename {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const CircuitAction&) const -> bool = default;
};

static_assert(std::regular<CircuitAction>);

struct ModalState {
    explicit ModalState() = default;
    explicit ModalState(ModalRequest request, FileAction action,
                        const circuit_ui_model::CircuitStore& circuit_store,
                        const CircuitUIConfig& config);

    ModalRequest request {};
    FileAction action {};

#ifdef _DEBUG
    // set at the start of modal action to guarantee that circuit is not changed
    std::string serialized_ {};
#endif

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const ModalState&) const -> bool = default;
};

static_assert(std::regular<ModalState>);

}  // namespace circuit_ui_model

template <>
[[nodiscard]] auto format(circuit_ui_model::UserAction action) -> std::string;

template <>
[[nodiscard]] auto format(circuit_ui_model::FileAction action) -> std::string;

}  // namespace logicsim

template <>
struct fmt::formatter<logicsim::circuit_ui_model::NameOrPath> {
    constexpr static auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::circuit_ui_model::NameOrPath& obj,
                       fmt::format_context& ctx) {
        const auto str = std::visit([](auto&& v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

template <>
struct fmt::formatter<logicsim::circuit_ui_model::ModalRequest> {
    constexpr static auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::circuit_ui_model::ModalRequest& obj,
                       fmt::format_context& ctx) {
        const auto str = std::visit([](auto&& v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

template <>
struct fmt::formatter<logicsim::circuit_ui_model::ModalResult> {
    constexpr static auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::circuit_ui_model::ModalResult& obj,
                       fmt::format_context& ctx) {
        const auto str = std::visit([](auto&& v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

template <>
struct fmt::formatter<logicsim::circuit_ui_model::ErrorMessage> {
    constexpr static auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::circuit_ui_model::ErrorMessage& obj,
                       fmt::format_context& ctx) {
        const auto str = std::visit([](auto&& v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

template <>
struct fmt::formatter<logicsim::circuit_ui_model::NextActionStep> {
    constexpr static auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::circuit_ui_model::NextActionStep& obj,
                       fmt::format_context& ctx) {
        const auto str = std::visit([](auto&& v) { return fmt::format("{}", v); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

namespace logicsim {

struct UnexpectedModalResultException : std::runtime_error {
    inline explicit UnexpectedModalResultException(
        const circuit_ui_model::ModalRequest& request,
        const circuit_ui_model::ModalResult& result)
        : std::runtime_error {
              fmt::format("Unexpected result {} to request {}.", result, request)} {};
};

/**
 * @brief: Circuit UI model that hold the circuit and coordinates
 *         managing rendering, simulation and user interactions.
 *
 * Class Invariants:
 *     + configs are the same as for CircuitWidget as all its sub-components
 *     + timer_benchmark_render_ is only active for render_config_.do_benchmark
 *     + timer_run_simulation_ is only active when in simulation state
 *     + setting dialog count is zero if not in editing state
 *     + layout contains only normal display state items if no editing is active
 *     + while modal is the set circuit does not change
 */
class CircuitUIModel {
   public:
    using Statistics = circuit_ui_model::Statistics;
    using UserAction = circuit_ui_model::UserAction;

    using UnsavedName = circuit_ui_model::UnsavedName;
    using SavedPath = circuit_ui_model::SavedPath;
    using SaveInformation = circuit_ui_model::SaveInformation;

    using FileAction = circuit_ui_model::FileAction;
    using ModalRequest = circuit_ui_model::ModalRequest;
    using ModalResult = circuit_ui_model::ModalResult;
    using FileActionResult = circuit_ui_model::FileActionResult;

   public:
    [[nodiscard]] explicit CircuitUIModel();

    // read methods
    [[nodiscard]] auto config() const -> const CircuitUIConfig&;
    [[nodiscard]] auto view_config() const -> const ViewConfig&;
    [[nodiscard]] auto history_status() const -> HistoryStatus;
    [[nodiscard]] auto allocation_info() const -> CircuitWidgetAllocInfo;
    [[nodiscard]] auto statistics() const -> Statistics;
    [[nodiscard]] auto layout() const -> const Layout&;
    [[nodiscard]] auto display_filename() const -> std::filesystem::path;

    // render
    auto render(BLImage& bl_image, device_pixel_ratio_t device_pixel_ratio) -> void;

    // load & save
    /**
     * Change ciruit itself (save, open, close).
     *
     * If this method is returning a ModalRequest the circuit-ui is in a modal
     * state. In this state no modifications are allowed to the model and most
     * methods throw exceptions (mouse & key events, config changes).
     *
     * Rendering is allowed as well as all read operations.
     */
    [[nodiscard]] auto file_action(FileAction action) -> FileActionResult;
    [[nodiscard]] auto submit_modal_result(const ModalResult& result) -> FileActionResult;

    [[nodiscard]] auto set_config(const CircuitUIConfig& config) -> UIStatus;
    [[nodiscard]] auto do_action(UserAction action,
                                 std::optional<point_device_fine_t> position) -> UIStatus;

    [[nodiscard]] auto mouse_press(const MousePressEvent& event) -> UIStatus;
    [[nodiscard]] auto mouse_move(const MouseMoveEvent& event) -> UIStatus;
    [[nodiscard]] auto mouse_release(const MouseReleaseEvent& event) -> UIStatus;
    [[nodiscard]] auto mouse_wheel(const MouseWheelEvent& event) -> UIStatus;
    [[nodiscard]] auto key_press(VirtualKey key) -> UIStatus;

   protected:
    // Q_SLOT void on_timer_benchmark_render();
    // Q_SLOT void on_timer_run_simulation();
    // Q_SLOT void on_setting_dialog_cleanup_request();
    // Q_SLOT void on_setting_dialog_attributes_changed(selection_id_t selection_id,
    //                                                  const SettingAttributes&
    //                                                  attributes);

    // auto resizeEvent(QResizeEvent* event_) -> void override;
    // auto renderEvent(BLImage bl_image, device_pixel_ratio_t device_pixel_ratio,
    //                  RenderMode render_mode,
    //                  fallback_info_t fallback_info) -> void override;
    //
    // auto mousePressEvent(QMouseEvent* event_) -> void override;
    // auto mouseMoveEvent(QMouseEvent* event_) -> void override;
    // auto mouseReleaseEvent(QMouseEvent* event_) -> void override;
    //
    // auto wheelEvent(QWheelEvent* event_) -> void override;
    // auto keyPressEvent(QKeyEvent* event_) -> void override;

   private:
    [[nodiscard]] auto set_editable_circuit(
        EditableCircuit&& editable_circuit, std::optional<ViewPoint> view_point = {},
        std::optional<SimulationConfig> simulation_config = {}) -> UIStatus;
    [[nodiscard]] auto set_save_information(SaveInformation&& save_information)
        -> UIStatus;
    [[nodiscard]] auto abort_current_action() -> UIStatus;
    [[nodiscard]] auto finalize_editing() -> UIStatus;
    [[nodiscard]] auto close_all_setting_dialogs() -> UIStatus;

    [[nodiscard]] auto do_modal_action(
        std::optional<circuit_ui_model::CircuitAction>& current_action,
        std::optional<circuit_ui_model::NextActionStep>& next_step) -> UIStatus;
    auto next_modal_action(circuit_ui_model::FileAction action,
                           std::optional<circuit_ui_model::NextActionStep>& next_step,
                           std::optional<circuit_ui_model::CircuitAction>& current_action)
        -> void;

    [[nodiscard]] auto load_new_circuit() -> UIStatus;
    [[nodiscard]] auto load_circuit_example(int number) -> UIStatus;
    [[nodiscard]] auto save_to_file(const std::filesystem::path& filename, bool& success)
        -> UIStatus;
    [[nodiscard]] auto open_from_file(const std::filesystem::path& filename,
                                      std::optional<LoadError>& load_error) -> UIStatus;

    auto undo() -> void;
    auto redo() -> void;
    auto update_history_status() -> void;
    auto select_all() -> void;
    auto delete_selected() -> void;
    [[nodiscard]] auto copy_paste_position() -> point_t;
    auto copy_selected() -> void;
    auto paste_clipboard() -> void;
    [[nodiscard]] auto zoom(double steps, std::optional<point_device_fine_t> position)
        -> UIStatus;
    [[nodiscard]] auto log_mouse_position(std::string_view source,
                                          point_device_fine_t position) -> UIStatus;

    [[nodiscard]] auto class_invariant_holds() const -> bool;
    /* only at the end of mutable methods, except paintEvent */
    [[nodiscard]] auto expensive_invariant_holds() const -> bool;

   private:
    // never modify the config directly, call set_config so sub-components are updated
    CircuitUIConfig config_ {};
    // call set_save_information
    SaveInformation save_information_ {};
    std::optional<circuit_ui_model::ModalState> modal_ {};

    circuit_ui_model::CircuitStore circuit_store_ {};
    circuit_ui_model::CircuitRenderer circuit_renderer_ {};
    circuit_ui_model::MouseDragLogic mouse_drag_logic_ {};
    circuit_ui_model::EditingLogicManager editing_logic_manager_;
    circuit_ui_model::DialogManager dialog_manager_;

    bool simulation_image_update_pending_ {false};
};

//
// CircuitWidgetState
//

[[nodiscard]] auto set_circuit_state(CircuitUIModel& model, CircuitWidgetState value)
    -> UIStatus;

//
// RenderConfig
//

[[nodiscard]] auto set_render_config(CircuitUIModel& model, WidgetRenderConfig value)
    -> UIStatus;

//
// SimulationConfig
//

[[nodiscard]] auto set_simulation_config(CircuitUIModel& model, SimulationConfig value)
    -> UIStatus;

}  // namespace logicsim

#endif
