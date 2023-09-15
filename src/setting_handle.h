#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "editable_circuit/selection_registrar.h"
#include "resource.h"
#include "vocabulary.h"

#include <ankerl/unordered_dense.h>
#include <blend2d.h>

#include <QDoubleValidator>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include <optional>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QLabel;
class QLayout;
class QFormLayout;

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
    auto set_attributes(QWidget* widget, layout::attributes_clock_generator attrs)
        -> void;

   private:
    [[nodiscard]] auto get_element_id(QWidget* dialog) const -> element_id_t;
    auto on_dialog_destroyed(QObject* object) -> void;
    auto on_cleanup_timeout() -> void;

   private:
    QWidget* parent_;
    EditableCircuit& editable_circuit_;
    QTimer cleanup_timer_;

    ankerl::unordered_dense::map<QWidget*, selection_handle_t> map_;
};

class AttributeSetter {
   public:
    explicit AttributeSetter(SettingWidgetRegistry* receiver);

    auto set_attributes(QWidget* sender, layout::attributes_clock_generator attrs)
        -> void;

   private:
    QPointer<SettingWidgetRegistry> receiver_ {};
};

//
// Clock Generator Dialog
//

class PeriodInput : public QObject {
   public:
    explicit PeriodInput(QWidget* parent, QString text, delay_t initial_value,
                         double scale_);

    auto value_changed() -> void;
    auto period_unit_changed() -> void;

    double scale;
    delay_t last_valid_period;

    QLineEdit* period_value;
    QComboBox* period_unit;
    QDoubleValidator period_validator;

    QLabel* label;
    QLayout* layout;
};

class ClockGeneratorDialog : public QWidget {
   public:
    explicit ClockGeneratorDialog(QWidget* parent, AttributeSetter setter,
                                  layout::ConstElement element);

   private:
    auto value_changed() -> void;
    auto update_row_visibility() -> void;

   private:
    AttributeSetter setter_;

    QFormLayout* layout_;

    QLineEdit* name_;
    QCheckBox* symmetric_period_;
    PeriodInput* period_;
    PeriodInput* period_on_;
    PeriodInput* period_off_;
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