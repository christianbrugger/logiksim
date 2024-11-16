
set(LS_TEST_SOURCES
    algorithm/fmt_join_test.cpp
    algorithm/range_extended_test.cpp
    algorithm/range_step_test.cpp
    algorithm/range_test.cpp
    algorithm/shuffle_test.cpp
    algorithm/transform_to_vector_test.cpp
    algorithm/uniform_int_distribution_test.cpp

    component/simulation/history_index_test.cpp
    component/simulation/history_view_test.cpp
    component/simulation/simulation_event_test.cpp

    container/graph/depth_first_search_test.cpp
    container/circular_buffer_test.cpp
    container/static_vector_test.cpp
    container/value_pointer_pimpl.cpp
    container/value_pointer_pimpl_test.cpp
    container/value_pointer_test.cpp

    editable_circuit/modifier/logicitem_test.cpp
    editable_circuit/modifier/modifier_config_test.cpp
    editable_circuit/modifier/wire_history_test.cpp
    editable_circuit/modifier/wire_random_test.cpp
    editable_circuit/modifier/wire_test.cpp
    editable_circuit/editable_circuit_random_test.cpp

    element/logicitem/layout_test.cpp
    element/logicitem/schematic_info_test.cpp

    format/container_test.cpp

    geometry/connection_count_test.cpp
    geometry/line_test.cpp
    geometry/part_selections.cpp
    geometry/part_test.cpp

    iterator_adaptor/enumerate_test.cpp
    iterator_adaptor/enumerate_transform_view_test.cpp
    iterator_adaptor/polling_iterator_test.cpp
    iterator_adaptor/transform_view_test.cpp

    random/generator_test.cpp

    vocabulary/color_test.cpp
    vocabulary/connection_count_test.cpp
    vocabulary/connection_id_test.cpp
    vocabulary/delay_test.cpp
    vocabulary/element_id_test.cpp
    vocabulary/grid_fine_test.cpp
    vocabulary/grid_test.cpp
    vocabulary/line_fine_test.cpp
    vocabulary/line_test.cpp
    vocabulary/offset_test.cpp
    vocabulary/ordered_line_test.cpp
    vocabulary/part_test.cpp
    vocabulary/point_fine_test.cpp
    vocabulary/point_test.cpp
    vocabulary/rect_fine_test.cpp
    vocabulary/rect_test.cpp
    vocabulary/segment_index_test.cpp
    vocabulary/segment_test.cpp
    vocabulary/time_test.cpp

    copy_paste_clipboard_test.cpp
    file_test.cpp
    layout_test.cpp
    line_tree_test.cpp
    load_save_file_test.cpp
    part_selection_test.cpp
    schematic_test.cpp
    segment_tree_test.cpp
    simulation_test.cpp
    timer_test.cpp
)

