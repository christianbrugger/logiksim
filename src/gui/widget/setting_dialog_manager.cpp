#include "gui/widget/setting_dialog_manager.h"

#include "gui/widget/setting_dialog.h"

#include "core/algorithm/overload.h"
#include "core/editable_circuit.h"

#include <QWidget>

namespace logicsim {

//
// Setting Widget Registrar
//

SettingDialogManager::SettingDialogManager(QWidget* parent)
    : QObject {parent}, parent_ {parent} {
    // Run timer, so dialogs with deleted logicitems are closed periodically.
    // It is advised to call cleanup whenever items might have been deleted.
    // This is a reliable fallback method, that catches any other case.
    connect(&timer_request_cleanup_, &QTimer::timeout, this,
            &SettingDialogManager::on_timer_request_cleanup);
    timer_request_cleanup_.setInterval(250);  // ms

    Ensures(class_invariant_holds());
}

namespace {

auto get_selected_element(const EditableCircuit& editable_circuit,
                          selection_id_t selection_id)
    -> std::optional<std::variant<logicitem_id_t, decoration_id_t>> {
    if (editable_circuit.selection_exists(selection_id)) {
        if (const auto logicitem_id =
                get_single_logicitem(editable_circuit.selection(selection_id))) {
            Ensures(logicitem_id);
            return logicitem_id;
        }
        if (const auto decoration_id =
                get_single_decoration(editable_circuit.selection(selection_id))) {
            Ensures(decoration_id);
            return decoration_id;
        }
    }
    return std::nullopt;
}

auto create_setting_dialog_element(const Layout& layout, logicitem_id_t logicitem_id,
                                   selection_id_t selection_id,
                                   QWidget* parent) -> SettingDialog* {
    switch (layout.logicitems().type(logicitem_id)) {
        case LogicItemType::clock_generator: {
            return new ClockGeneratorDialog {
                parent, selection_id,
                layout.logicitems().attrs_clock_generator(logicitem_id)};
        }

        default: {
            throw std::runtime_error("type doesn't have dialog");
        }
    }
}

auto create_setting_dialog_element(const Layout& layout, decoration_id_t decoration_id,
                                   selection_id_t selection_id,
                                   QWidget* parent) -> SettingDialog* {
    switch (layout.decorations().type(decoration_id)) {
        case DecorationType::text_element: {
            return new TextElementDialog {
                parent, selection_id,
                layout.decorations().attrs_text_element(decoration_id)};
        }

        default: {
            throw std::runtime_error("type doesn't have dialog");
        }
    }
}

auto create_setting_dialog(const EditableCircuit& editable_circuit,
                           selection_id_t selection_id,
                           QWidget* parent) -> SettingDialog* {
    const auto element = get_selected_element(editable_circuit, selection_id);
    if (!element) {
        throw std::runtime_error("Selection must hold exactly one element");
    }

    return std::visit(
        [&](auto element_id) {
            return create_setting_dialog_element(editable_circuit.layout(), element_id,
                                                 selection_id, parent);
        },
        element.value());
}

}  // namespace

auto SettingDialogManager::show_setting_dialog(
    EditableCircuit& editable_circuit,
    std::variant<logicitem_id_t, decoration_id_t> element_id) -> void {
    Expects(class_invariant_holds());

    // find existing dialog
    for (auto&& [selection_id, widget] : map_) {
        if (const auto element = get_selected_element(editable_circuit, selection_id);
            element && *element == element_id) {
            widget->show();
            widget->activateWindow();

            Ensures(class_invariant_holds());
            return;
        }
    }

    // created tracked selection
    const auto selection_id = editable_circuit.create_selection();
    Expects(selection_id);

    try {
        std::visit(
            [&](auto element_id_) {
                editable_circuit.add_to_selection(selection_id, element_id_);
            },
            element_id);
        const auto [it, inserted] = map_.emplace(selection_id, nullptr);
        Expects(inserted);

    } catch (...) {
        editable_circuit.destroy_selection(selection_id);
        throw;
    }

    // create dialog
    auto* widget = create_setting_dialog(editable_circuit, selection_id, parent_);
    Expects(widget);

    try {
        if (!connect(widget, &QWidget::destroyed, this,
                     &SettingDialogManager::on_dialog_destroyed) ||
            !connect(widget, &SettingDialog::attributes_changed, this,
                     &SettingDialogManager::on_dialog_attributes_changed)) {
            throw std::runtime_error("unable to setup dialog connection");
        };

        map_.at(selection_id) = widget;
    } catch (...) {
        widget->deleteLater();
        throw;
    }

    widget->show();

    // start timer, as we have now at least one active dialog
    timer_request_cleanup_.start();

    Ensures(class_invariant_holds());
}

auto SettingDialogManager::close_all(EditableCircuit& editable_circuit) -> void {
    Expects(class_invariant_holds());

    for (auto&& [_, widget] : map_) {
        if (widget != nullptr) {
            widget->deleteLater();
            widget = nullptr;
        }
    }
    run_cleanup(editable_circuit);

    Ensures(class_invariant_holds());
}

auto SettingDialogManager::run_cleanup(EditableCircuit& editable_circuit) -> void {
    Expects(class_invariant_holds());

    // close dialogs with deleted logic-items
    for (auto&& [selection_id, widget] : map_) {
        if (widget != nullptr) {
            if (!get_selected_element(editable_circuit, selection_id)) {
                widget->deleteLater();
                widget = nullptr;
            }
        }
    }

    // find destroyed dialogs
    auto delete_list = std::vector<selection_id_t> {};
    for (auto&& [selection_id, widget] : map_) {
        if (widget == nullptr) {
            delete_list.push_back(selection_id);
        }
    }

    // free tracked-selections
    for (const auto& selection_id : delete_list) {
        save_destroy_selection(editable_circuit, selection_id);
        Expects(map_.erase(selection_id) == 1);
    }

    // setup timer
    if (map_.empty()) {
        timer_request_cleanup_.stop();
    }

    Ensures(class_invariant_holds());
}

auto SettingDialogManager::open_dialog_count() const -> std::size_t {
    Expects(class_invariant_holds());

    return map_.size();
}

Q_SLOT void SettingDialogManager::on_dialog_destroyed(QObject* object) {
    Expects(class_invariant_holds());

    const auto dialogs = std::ranges::views::values(map_);
    const auto it = std::ranges::find(dialogs, object);

    if (it != dialogs.end()) {
        *it = nullptr;
        Q_EMIT request_cleanup();
    }

    Ensures(class_invariant_holds());
}

Q_SLOT void SettingDialogManager::on_dialog_attributes_changed(
    selection_id_t selection_id, const SettingAttributes& attributes) {
    Expects(class_invariant_holds());

    Q_EMIT attributes_changed(selection_id, attributes);
}

Q_SLOT void SettingDialogManager::on_timer_request_cleanup() {
    Expects(class_invariant_holds());

    Q_EMIT request_cleanup();
}

auto SettingDialogManager::class_invariant_holds() const -> bool {
    Expects(timer_request_cleanup_.isActive() == !map_.empty());

    return true;
}

//
// Public Methods
//

namespace {

auto change_setting_attributes_element(EditableCircuit& editable_circuit,
                                       logicitem_id_t logicitem_id,
                                       const SettingAttributes& attributes) -> void {
    switch (editable_circuit.layout().logicitems().type(logicitem_id)) {
        case LogicItemType::clock_generator: {
            editable_circuit.set_attributes(
                logicitem_id, std::get<attributes_clock_generator_t>(attributes));
            return;
        }

        default: {
            throw std::runtime_error("Unsupported type");
        }
    }
}

auto change_setting_attributes_element(EditableCircuit& editable_circuit,
                                       decoration_id_t decoration_id,
                                       const SettingAttributes& attributes) -> void {
    switch (editable_circuit.layout().decorations().type(decoration_id)) {
        case DecorationType::text_element: {
            editable_circuit.set_attributes(
                decoration_id, std::get<attributes_text_element_t>(attributes));
            return;
        }

        default: {
            throw std::runtime_error("Unsupported type");
        }
    }
}

}  // namespace

auto change_setting_attributes(EditableCircuit& editable_circuit,
                               selection_id_t selection_id,
                               const SettingAttributes& attributes) -> void {
    const auto element = get_selected_element(editable_circuit, selection_id);
    if (!element) {
        return;
    }

    std::visit(
        [&](auto element_id) {
            change_setting_attributes_element(editable_circuit, element_id, attributes);
        },
        element.value());

    editable_circuit.finish_undo_group();
}

}  // namespace logicsim
