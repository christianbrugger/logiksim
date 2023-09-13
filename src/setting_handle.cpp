#include "setting_handle.h"

#include "collision.h"
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

MouseSettingHandleLogic::MouseSettingHandleLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit},
      setting_handle_ {args.setting_handle},
      parent_ {args.parent} {}

auto MouseSettingHandleLogic::mouse_press(point_fine_t position) -> void {
    first_position_ = position;
}

auto MouseSettingHandleLogic::mouse_release(point_fine_t position) -> void {
    if (first_position_ && is_colliding(setting_handle_, first_position_.value()) &&
        is_colliding(setting_handle_, position)) {
        print("clicked on setting handle");

        static auto test_counter = 0;  // TODO !!! REMOVE
        ++test_counter;

        if (test_counter % 2 == 0) {
            auto* widget = new ClockGeneratorWidget(parent_);
            widget->setWindowFlags(Qt::Dialog);
            widget->setWindowTitle(widget->tr("Clock Generator"));
            widget->setWindowIcon(QIcon(get_icon_path(setting_handle_.icon)));
            auto* layout = new QFormLayout(widget);

            // Name
            {
                auto* label = new QLabel(widget);
                label->setText(widget->tr("Clock Name:"));
                auto* text_edit = new QLineEdit(widget);
                text_edit->setText(widget->tr("Clock 1"));

                layout->addRow(label, text_edit);
            }

            // Period
            {
                auto* label = new QLabel(widget);
                label->setText(widget->tr("Clock Period:"));

                auto* hlayout = new QHBoxLayout(widget);
                auto* spin_box = new QSpinBox(widget);
                auto* combo_box = new QComboBox(widget);

                spin_box->setMinimum(1);
                spin_box->setMaximum(1000);

                combo_box->addItems({
                    widget->tr("ns"),
                    widget->tr("µs"),
                    widget->tr("ms"),
                    widget->tr("seconds"),
                });

                hlayout->addWidget(spin_box);
                hlayout->addWidget(combo_box);

                layout->addRow(label, hlayout);
            }

            // Simulation Controls
            {
                auto* check_box = new QCheckBox(widget);
                check_box->setText(widget->tr("Show Simulation Controls"));
                check_box->setChecked(true);

                layout->addRow(nullptr, check_box);
            }

            widget->show();
        } else {
            auto* widget = new ClockGeneratorWidget(parent_);
            widget->setWindowFlags(Qt::Dialog);
            widget->setWindowTitle(widget->tr("Clock Generator"));
            widget->setWindowIcon(QIcon(get_icon_path(setting_handle_.icon)));
            auto* layout = new QVBoxLayout(widget);

            // Name
            {
                auto* label = new QLabel(widget);
                label->setText(widget->tr("Clock Name:"));
                auto* text_edit = new QLineEdit(widget);
                text_edit->setText(widget->tr("Clock 1"));

                layout->addWidget(label);
                layout->addWidget(text_edit);
            }

            // Period
            {
                auto* label = new QLabel(widget);
                label->setText(widget->tr("Clock Period:"));

                layout->addWidget(label);
            }
            {
                auto* hlayout = new QHBoxLayout(widget);
                auto* spin_box = new QSpinBox(widget);
                auto* combo_box = new QComboBox(widget);

                spin_box->setMinimum(1);
                spin_box->setMaximum(1000);

                combo_box->addItems({
                    widget->tr("ns"),
                    widget->tr("µs"),
                    widget->tr("ms"),
                    widget->tr("seconds"),
                });

                hlayout->addWidget(spin_box);
                hlayout->addWidget(combo_box);

                layout->addLayout(hlayout);
            }

            // Simulation Controls
            {
                auto* check_box = new QCheckBox(widget);
                check_box->setText(widget->tr("Show Simulation Controls"));
                check_box->setChecked(true);

                layout->addWidget(check_box);
            }

            widget->show();
        }
    }
}

}  // namespace logicsim