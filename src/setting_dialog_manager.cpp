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

auto get_selected_logic_item(const EditableCircuit& editable_circuit,
                             selection_id_t selection_id) -> logicitem_id_t {
    if (!editable_circuit.selection_exists(selection_id)) {
        return null_logicitem_id;
    }
    const auto& selection = editable_circuit.selection(selection_id);

    if (selection.selected_logic_items().size() != 1 ||
        selection.selected_segments().size() != 0) {
        return null_logicitem_id;
    }

    return selection.selected_logic_items().front();
}

auto create_setting_dialog(const EditableCircuit& editable_circuit,
                           selection_id_t selection_id, QWidget* parent)
    -> SettingDialog* {
    const auto logicitem_id = get_selected_logic_item(editable_circuit, selection_id);
    Expects(logicitem_id);

    const auto logicitem_type =
        editable_circuit.layout().logic_items().type(logicitem_id);

    if (logicitem_type == LogicItemType::clock_generator) {
        return new ClockGeneratorDialog {
            parent, selection_id,
            editable_circuit.layout().logic_items().attrs_clock_generator(logicitem_id)};
    }

    throw std::runtime_error("type doesn't have dialog");
}

}  // namespace

auto SettingDialogManager::show_setting_dialog(EditableCircuit& editable_circuit,
                                               setting_handle_t setting_handle) -> void {
    // find existing dialog
    for (auto&& [selection_id, widget] : map_) {
        if (get_selected_logic_item(editable_circuit, selection_id) ==
            setting_handle.logicitem_id) {
            widget->show();
            widget->activateWindow();
            return;
        }
    }

    // created tracked selection
    const auto selection_id = editable_circuit.create_selection();
    Expects(selection_id);

    try {
        editable_circuit.selection(selection_id).add(setting_handle.logicitem_id);
        const auto [it, inserted] = map_.emplace(selection_id, nullptr);
        Expects(inserted);

    } catch (std::exception&) {
        editable_circuit.destroy_selection(selection_id);
        throw;
    }

    // create dialog
    auto* widget = create_setting_dialog(editable_circuit, selection_id, parent_);
    Expects(widget);

    try {
        connect(widget, &QWidget::destroyed, this,
                &SettingDialogManager::on_dialog_destroyed);
        connect(widget, &SettingDialog::attributes_changed, this,
                &SettingDialogManager::on_dialog_attributes_changed);
        map_.at(selection_id) = widget;
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
    // close dialogs with deleted logic-items
    for (auto&& [selection_id, widget] : map_) {
        if (widget) {
            if (!get_selected_logic_item(editable_circuit, selection_id)) {
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
    for (const auto& selection_id : delete_list) {
        save_destroy_selection(editable_circuit, selection_id);
        Expects(map_.erase(selection_id) == 1);
    }
}

Q_SLOT void SettingDialogManager::on_dialog_destroyed(QObject* object) {
    const auto dialogs = std::ranges::views::values(map_);
    const auto it = std::ranges::find(dialogs, object);

    if (it != dialogs.end()) {
        *it = nullptr;
        Q_EMIT request_cleanup();
    }
}

Q_SLOT void SettingDialogManager::on_dialog_attributes_changed(
    selection_id_t selection_id, SettingAttributes attributes) {
    Q_EMIT attributes_changed(selection_id, attributes);
}

//
// Public Methods
//

auto change_setting_attributes(EditableCircuit& editable_circuit,
                               selection_id_t selection_id, SettingAttributes attributes)
    -> void {
    const auto element_id = get_selected_logic_item(editable_circuit, selection_id);

    if (!element_id) {
        return;
    }

    const auto logicitem_type = editable_circuit.layout().logic_items().type(element_id);

    if (logicitem_type == LogicItemType::clock_generator &&
        attributes.attrs_clock_generator) {
        editable_circuit.set_attributes(element_id,
                                        attributes.attrs_clock_generator.value());
    }
}

}  // namespace logicsim