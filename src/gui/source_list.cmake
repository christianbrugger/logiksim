
# TODO move component to core project
set(LS_GUI_SOURCES
    component/circuit_widget/mouse_logic/editing_logic_concept.cpp
    component/circuit_widget/mouse_logic/editing_logic_manager.cpp
    component/circuit_widget/mouse_logic/editing_logic_variant.cpp
    component/circuit_widget/mouse_logic/handle_resize.cpp
    component/circuit_widget/mouse_logic/handle_setting.cpp
    component/circuit_widget/mouse_logic/insert_decoration.cpp
    component/circuit_widget/mouse_logic/insert_logicitem.cpp
    component/circuit_widget/mouse_logic/insert_wire.cpp
    component/circuit_widget/mouse_logic/mouse_drag_logic.cpp
    component/circuit_widget/mouse_logic/mouse_wheel_logic.cpp
    component/circuit_widget/mouse_logic/selection_area.cpp
    component/circuit_widget/mouse_logic/selection_move.cpp
    component/circuit_widget/mouse_logic/selection_single.cpp
    component/circuit_widget/checked_editable_circuit.cpp
    component/circuit_widget/circuit_renderer.cpp
    component/circuit_widget/circuit_store.cpp
    component/circuit_widget/simulation_runner.cpp
    component/circuit_widget/zoom.cpp

    format/qt_type.cpp

    qt/clipboard_access.cpp
    qt/enum_to_string.cpp
    qt/mouse_position.cpp
    qt/mouse_position_p.cpp
    qt/path_conversion.cpp
    qt/point_conversion.cpp
    qt/setting_location.cpp
    qt/svg_icon_engine.cpp
    qt/widget_geometry.cpp

    widget/circuit_widget.cpp
    widget/circuit_widget_base.cpp
    widget/render_widget.cpp
    widget/setting_dialog.cpp
    widget/setting_dialog_manager.cpp
    widget/top_widget.cpp
)

