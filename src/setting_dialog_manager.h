#ifndef LOGICSIM_SETTING_DIALOG_MANAGER_H
#define LOGICSIM_SETTING_DIALOG_MANAGER_H

#include "vocabulary/selection_id.h"
#include "vocabulary/setting_attribute.h"

#include <ankerl/unordered_dense.h>

#include <QObject>

class QWidget;

namespace logicsim {

struct setting_handle_t;
struct logicitem_id_t;
struct LogicItemDefinition;

class EditableCircuit;
class SettingDialog;

//
// Setting Dialog Manager
//

class SettingDialogManager : public QObject {
    Q_OBJECT

   public:
    explicit SettingDialogManager(QWidget* parent);

    auto show_setting_dialog(EditableCircuit& editable_circuit,
                             setting_handle_t setting_handle) -> void;
    auto close_all(EditableCircuit& editable_circuit) -> void;
    auto run_cleanup(EditableCircuit& editable_circuit) -> void;

   public:
    Q_SIGNAL void attributes_changed(selection_id_t selection_id,
                                     SettingAttributes attributes);
    Q_SIGNAL void request_cleanup();

   private:
    Q_SLOT void on_dialog_destroyed(QObject* object);
    Q_SLOT void on_attributes_changed(selection_id_t selection_id,
                                      SettingAttributes attributes);

   private:
    QWidget* parent_;

    ankerl::unordered_dense::map<selection_id_t, SettingDialog*> map_;
};

//
// Public Methods
//

auto change_setting_attributes(EditableCircuit& editable_circuit,
                               selection_id_t selection_id, SettingAttributes attributes)
    -> void;

}  // namespace logicsim

#endif
