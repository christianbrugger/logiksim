﻿#include "setting_handle.h"

#include "collision.h"
#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection.h"
#include "layout.h"
#include "layout_calculation.h"
#include "resource.h"
#include "scene.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace logicsim {
auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> std::optional<setting_handle_t> {
    const auto element = layout.element(element_id);

    switch (layout.element_type(element_id)) {
        using enum ElementType;

        case clock_generator: {
            constexpr auto overdraw = defaults::logic_item_body_overdraw;
            constexpr auto handle_size = defaults::setting_handle_size;
            constexpr auto margin = defaults::setting_handle_margin;

            const auto width = 3.0;
            const auto height = 2.0;

            return setting_handle_t {
                .position = transform(element.position(), element.orientation(),
                                      point_fine_t {
                                          width - handle_size / 2.0 - margin,
                                          height + overdraw - handle_size / 2.0 - margin,
                                      }),
                .icon = icon_t::setting_handle_clock_generator,
                .element_id = element_id,
            };
        }

        case unused:
        case placeholder:
        case wire:

        case buffer_element:
        case and_element:
        case or_element:
        case xor_element:

        case button:
        case led:
        case display_number:
        case display_ascii:

        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:

        case sub_circuit:
            return {};
    };

    throw_exception("unknown ElementType in setting_handle_position");
}

namespace {

auto get_single_logic_item(const Selection& selection) -> element_id_t {
    if (selection.selected_logic_items().size() != 1 ||
        !selection.selected_segments().empty()) {
        return null_element;
    }
    return selection.selected_logic_items().front();
}

}  // namespace

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto element_id = get_single_logic_item(selection);
    if (!element_id) {
        return {};
    }
    if (layout.display_state(element_id) != display_state_t::normal) {
        return {};
    }

    return setting_handle_position(layout, element_id);
}

auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t {
    return to_rect(handle.position, defaults::setting_handle_size);
}

auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool {
    return is_colliding(position, setting_handle_rect(handle));
}

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto handle = setting_handle_position(layout, selection);

    if (handle && is_colliding(*handle, position)) {
        return handle;
    }

    return {};
}

//
// Setting Widget Registrar
//

SettingWidgetRegistry::SettingWidgetRegistry(QWidget* parent,
                                             EditableCircuit& editable_circuit)
    : parent_ {parent}, editable_circuit_ {editable_circuit} {}

SettingWidgetRegistry::~SettingWidgetRegistry() {
    close_all();
}

auto SettingWidgetRegistry::show_setting_dialog(setting_handle_t setting_handle) -> void {
    // activate existing
    for (auto&& [widget, selection_handle] : map_) {
        if (selection_handle->is_selected(setting_handle.element_id)) {
            widget->show();
            widget->activateWindow();
            return;
        }
    }
    // create new
    const auto element = editable_circuit_.layout().element(setting_handle.element_id);
    auto* widget = new ClockGeneratorDialog {parent_, *this, element};
    widget->setWindowFlags(Qt::Dialog);
    widget->setAttribute(Qt::WA_DeleteOnClose);

    try {
        const auto [it, inserted] = map_.emplace(widget, editable_circuit_.get_handle());
        if (!inserted) {
            throw_exception("could not insert widget into map");
        }
        it->second.value().add_logicitem(setting_handle.element_id);

        connect(widget, &QWidget::destroyed, this,
                &SettingWidgetRegistry::on_dialog_destroyed);
    } catch (std::exception&) {
        widget->deleteLater();
        throw;
    }

    widget->show();
}

auto SettingWidgetRegistry::close_all() -> void {
    for (auto&& [widget, _] : map_) {
        disconnect(widget, &QWidget::destroyed, this,
                   &SettingWidgetRegistry::on_dialog_destroyed);
        delete widget;
    }
    map_.clear();
}

auto SettingWidgetRegistry::element_id(QWidget* dialog) const -> element_id_t {
    if (const auto it = map_.find(dialog); it != map_.end()) {
        const auto& selection_handle = it->second;
        const auto& selected_items = selection_handle.value().selected_logic_items();

        if (selected_items.size() != 1) {
            throw_exception("unexpected selected items size in SettingWidgetRegistry");
        }

        return selected_items.front();
    }
    throw_exception("dialog is not registered in SettingWidgetRegistry");
}

auto SettingWidgetRegistry::element(QWidget* dialog) const -> layout::ConstElement {
    return editable_circuit_.layout().element(element_id(dialog));
}

auto SettingWidgetRegistry::on_dialog_destroyed(QObject* object) -> void {
    auto* widget = reinterpret_cast<QWidget*>(object);
    if (!widget) {
        throw_exception("got non-widget object in dialog_destroyed");
    }
    if (map_.erase(widget) != 1) {
        throw_exception("did not find widget marked for deletion");
    }
    print("object erased", map_.size());
}

//
// Clock Generator Dialog
//

ClockGeneratorDialog::ClockGeneratorDialog(QWidget* parent,
                                           SettingWidgetRegistry& widget_registry,
                                           layout::ConstElement element)
    : QWidget(parent), widget_registry_ {widget_registry} {
    setWindowTitle(tr("Clock Generator"));
    setWindowIcon(QIcon(get_icon_path(icon_t::setting_handle_clock_generator)));

    const auto& attrs = element.attrs_clock_generator();

    auto* layout = new QFormLayout(this);

    // Name
    {
        auto* label = new QLabel(this);
        label->setText(tr("Clock Name:"));
        auto* text_edit = new QLineEdit(this);
        text_edit->setText(QString::fromStdString(attrs.name));

        layout->addRow(label, text_edit);
    }

    // Period
    {
        auto* label = new QLabel(this);
        label->setText(tr("Clock Period:"));

        auto* hlayout = new QHBoxLayout();
        auto* spin_box = new QSpinBox(this);
        auto* combo_box = new QComboBox(this);

        spin_box->setMinimum(1);
        spin_box->setMaximum(1000);

        combo_box->addItems({tr("ns"), tr("µs"), tr("ms"), tr("seconds")});

        hlayout->addWidget(spin_box);
        hlayout->addWidget(combo_box);

        layout->addRow(label, hlayout);
    }

    // Simulation Controls
    {
        auto* check_box = new QCheckBox(this);
        check_box->setText(tr("Show Simulation Controls"));
        check_box->setChecked(attrs.show_simulation_controls);

        layout->addRow(nullptr, check_box);
    }
}

//
// Mouse Setting Handle Logic
//

MouseSettingHandleLogic::MouseSettingHandleLogic(Args args) noexcept
    : widget_registry_ {args.widget_registry}, setting_handle_ {args.setting_handle} {}

auto MouseSettingHandleLogic::mouse_press(point_fine_t position) -> void {
    first_position_ = position;
}

auto MouseSettingHandleLogic::mouse_release(point_fine_t position) -> void {
    if (first_position_ && is_colliding(setting_handle_, first_position_.value()) &&
        is_colliding(setting_handle_, position)) {
        widget_registry_.show_setting_dialog(setting_handle_);
    }
}

}  // namespace logicsim