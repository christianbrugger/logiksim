#include "setting_dialog_manager.h"

#include "editable_circuit.h"
#include "logging.h"
#include "setting_dialog.h"
#include "setting_handle.h"
#include "vocabulary/logicitem_definition.h"

#include <QWidget>

namespace logicsim {

//
// Setting Widget Registrar
//

SettingDialogManager::SettingDialogManager(QWidget* parent)
    : QObject {parent}, parent_ {parent} {}

namespace {

auto create_setting_dialog(const EditableCircuit& editable_circuit,
                           setting_handle_t setting_handle, QWidget* parent,
                           selection_id_t selection_id) -> SettingDialog* {
    const auto logicitem_type =
        editable_circuit.layout().logic_items().type(setting_handle.logicitem_id);

    if (logicitem_type == LogicItemType::clock_generator) {
        return new ClockGeneratorDialog {
            parent, selection_id,
            editable_circuit.layout().logic_items().attrs_clock_generator(
                setting_handle.logicitem_id)};
    }

    throw std::runtime_error("type doesn't have dialog");
}

}  // namespace

auto SettingDialogManager::show_setting_dialog(EditableCircuit& editable_circuit,
                                               setting_handle_t setting_handle) -> void {
    // find existing dialog
    for (auto&& [selection_id, widget] : map_) {
        if (editable_circuit.selection_exists(selection_id) &&
            editable_circuit.selection(selection_id)
                .is_selected(setting_handle.logicitem_id)) {
            widget->show();
            widget->activateWindow();
            return;
        }
    }

    // create new dialog
    // TODO EXCEPTION SAFETY  !!!
    const auto selection_id = editable_circuit.create_selection();
    editable_circuit.selection(selection_id).add(setting_handle.logicitem_id);

    auto* widget =
        create_setting_dialog(editable_circuit, setting_handle, parent_, selection_id);
    Expects(widget);

    // add to map
    try {
        connect(widget, &QWidget::destroyed, this,
                &SettingDialogManager::on_dialog_destroyed);
        connect(widget, &SettingDialog::attributes_changed, this,
                &SettingDialogManager::on_attributes_changed);

        const auto [it, inserted] = map_.emplace(selection_id, widget);
        Expects(inserted);
    } catch (std::exception&) {
        widget->deleteLater();
        throw;
    }

    widget->show();
}

auto SettingDialogManager::close_all(EditableCircuit& editable_circuit) -> void {
    for (auto&& [_, widget] : map_) {
        if (widget) {
            widget->deleteLater();
            widget = nullptr;
        }
    }
    run_cleanup(editable_circuit);
}

auto SettingDialogManager::run_cleanup(EditableCircuit& editable_circuit) -> void {
    print("TODO EXCEPTION SAFETY");
    print("TODO close all dialogs & finalize editing when loading new circuit");

    // close dialogs with empty selections
    for (auto&& [selection_id, widget] : map_) {
        if (widget) {
            if (!editable_circuit.selection_exists(selection_id) ||
                editable_circuit.selection(selection_id).empty()) {
                widget->deleteLater();
                widget = nullptr;
            }
        }
    }

    // find destroyed dialogs
    auto delete_list = std::vector<selection_id_t> {};
    for (auto&& [selection_id, widget] : map_) {
        if (!widget) {
            delete_list.push_back(selection_id);
        }
    }

    // free selections
    for (auto selection_id : delete_list) {
        save_destroy_selection(editable_circuit, selection_id);
        Expects(map_.erase(selection_id) == 1);
    }
}

Q_SLOT void SettingDialogManager::on_dialog_destroyed(QObject* object) {
    // unregister dialog
    const auto values = std::ranges::views::values(map_);
    const auto it = std::ranges::find(values, object);

    if (it != values.end()) {
        *it = nullptr;
        Q_EMIT request_cleanup();
    }
}

Q_SLOT void SettingDialogManager::on_attributes_changed(selection_id_t selection_id,
                                                        SettingAttributes attributes) {
    Q_EMIT attributes_changed(selection_id, attributes);
}

//
// Public Methods
//

namespace {

auto get_selected_logic_item(const EditableCircuit& editable_circuit,
                             selection_id_t selection_id) -> logicitem_id_t {
    if (!editable_circuit.selection_exists(selection_id)) {
        return null_logicitem_id;
    }

    const auto& selection = editable_circuit.selection(selection_id);

    if (selection.selected_logic_items().size() > 1 ||
        selection.selected_segments().size() > 0) {
        throw std::runtime_error(
            "unexpected selected items size in SettingWidgetRegistry");
    }
    if (selection.selected_logic_items().size() == 0) {
        return null_logicitem_id;
    }

    return selection.selected_logic_items().front();
}

}  // namespace

auto change_setting_attributes(EditableCircuit& editable_circuit,
                               selection_id_t selection_id, SettingAttributes attributes)
    -> void {
    const auto element_id = get_selected_logic_item(editable_circuit, selection_id);

    if (!element_id) {
        return;
    }

    if (attributes.attrs_clock_generator) {
        editable_circuit.set_attributes(element_id,
                                        attributes.attrs_clock_generator.value());
    }
}

}  // namespace logicsim
