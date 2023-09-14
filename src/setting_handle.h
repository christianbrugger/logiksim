#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "editable_circuit/selection_registrar.h"
#include "resource.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <blend2d.h>

#include <QObject>
#include <QWidget>

#include <optional>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;

namespace logicsim {
class Layout;
class Selection;
class EditableCircuit;
class ViewConfig;

namespace layout {
template <bool Const>
class ElementTemplate;
using ConstElement = ElementTemplate<true>;
struct attributes_clock_generator;
}  // namespace layout

namespace defaults {
constexpr static inline auto setting_handle_size = grid_fine_t {1.0};
constexpr static inline auto setting_handle_margin = grid_fine_t {0.1};
}  // namespace defaults

struct setting_handle_t {
    point_fine_t position;
    icon_t icon;
    element_id_t element_id;
};

auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> std::optional<setting_handle_t>;

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t>;

auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t;

auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool;

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection)
    -> std::optional<setting_handle_t>;

//
// Settings Registrar
//

class SettingWidgetRegistry : public QObject {
   public:
    explicit SettingWidgetRegistry(QWidget* parent, EditableCircuit& editable_circuit);
    ~SettingWidgetRegistry();

    SettingWidgetRegistry(SettingWidgetRegistry&&) = default;
    SettingWidgetRegistry(const SettingWidgetRegistry&) = delete;
    auto operator=(SettingWidgetRegistry&&) -> SettingWidgetRegistry& = default;
    auto operator=(const SettingWidgetRegistry&) -> SettingWidgetRegistry& = delete;

    auto show_setting_dialog(setting_handle_t setting_handle) -> void;
    auto close_all() -> void;

    [[nodiscard]] auto element_id(QWidget* dialog) const -> element_id_t;
    [[nodiscard]] auto element(QWidget* dialog) const -> layout::ConstElement;

    auto set_attributes(QWidget* dialog, layout::attributes_clock_generator attrs)
        -> void;

   private:
    auto on_dialog_destroyed(QObject* object) -> void;

   private:
    QWidget* parent_;
    EditableCircuit& editable_circuit_;

    ankerl::unordered_dense::map<QWidget*, selection_handle_t> map_;
};

//
// Clock Generator Dialog
//

class ClockGeneratorDialog : public QWidget {
   public:
    explicit ClockGeneratorDialog(QWidget* parent, SettingWidgetRegistry& widget_registry,
                                  layout::ConstElement element);

   private:
    auto value_changed() -> void;

   private:
    SettingWidgetRegistry& widget_registry_;

    QLineEdit* name_;
    QSpinBox* period_value_;
    QComboBox* period_unit_;
    QCheckBox* simulation_controls_;
};

//
// Mouse Setting Handle Logic
//

class MouseSettingHandleLogic {
   public:
    struct Args {
        SettingWidgetRegistry& widget_registry;
        setting_handle_t setting_handle;
    };

    MouseSettingHandleLogic(Args args) noexcept;

    auto mouse_press(point_fine_t position) -> void;
    auto mouse_release(point_fine_t position) -> void;

   private:
    SettingWidgetRegistry& widget_registry_;
    setting_handle_t setting_handle_;

    std::optional<point_fine_t> first_position_ {};
};

}  // namespace logicsim

#endif