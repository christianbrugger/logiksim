#ifndef LOGICSIM_WIDGET_SETTING_DIALOG_MANAGER_H
#define LOGICSIM_WIDGET_SETTING_DIALOG_MANAGER_H

#include "core/vocabulary/decoration_definition.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/selection_id.h"
#include "core/vocabulary/setting_attribute.h"

#include <ankerl/unordered_dense.h>

#include <QObject>
#include <QTimer>

#include <variant>

class QWidget;

namespace logicsim {

struct logicitem_id_t;
struct decoration_id_t;
struct LogicItemDefinition;

class EditableCircuit;
class SettingDialog;

/**
 * @brief: Coordinates multiple settings dialogs for an editable circuit.
 *
 * Note that each dialog has a corresponding tracked-selection on the editable-circuit
 * in order to keep tracking the same element when its id changes.
 *
 * A consequence of this is that when a dialog is closed, a cleanup step is necessary
 * where this tracked-selection is destroyed. As the manager doesn't hold the editable
 * circuit, this is done by emitting a signal and an external call to cleanup.
 *
 * Class invariants:
 *   + cleanup timer is only running if the map has entries
 */
class SettingDialogManager : public QObject {
    Q_OBJECT

   public:
    explicit SettingDialogManager(QWidget* parent);

    auto show_setting_dialog(EditableCircuit& editable_circuit,
                             std::variant<logicitem_id_t, decoration_id_t> element_id)
        -> void;
    auto close_all(EditableCircuit& editable_circuit) -> void;
    auto run_cleanup(EditableCircuit& editable_circuit) -> void;

    [[nodiscard]] auto open_dialog_count() const -> std::size_t;

   public:
    Q_SIGNAL void attributes_changed(selection_id_t selection_id,
                                     const SettingAttributes& attributes);
    Q_SIGNAL void request_cleanup();

   private:
    Q_SLOT void on_dialog_destroyed(QObject* object);
    Q_SLOT void on_dialog_attributes_changed(selection_id_t selection_id,
                                             const SettingAttributes& attributes);
    Q_SLOT void on_timer_request_cleanup();

   private:
    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    QWidget* parent_ {};
    ankerl::unordered_dense::map<selection_id_t, SettingDialog*> map_ {};

    QTimer timer_request_cleanup_ {};
};

//
// Public Methods
//

auto change_setting_attributes(EditableCircuit& editable_circuit,
                               selection_id_t selection_id,
                               const SettingAttributes& attributes) -> void;

}  // namespace logicsim

#endif
