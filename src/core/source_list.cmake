
set(LS_CORE_SOURCES
    algorithm/accumulate.cpp
    algorithm/adjacent_count_if.cpp
    algorithm/all_equal.cpp
    algorithm/checked_deref.cpp
    algorithm/compare_sorted.cpp
    algorithm/contains.cpp
    algorithm/copy_adjacent_if.cpp
    algorithm/create_array.cpp
    algorithm/depth_first_visitor.cpp
    algorithm/distance_fast.cpp
    algorithm/fmt_join.cpp
    algorithm/has_duplicates_quadratic.cpp
    algorithm/make_unique.cpp
    algorithm/merged_any_of.cpp
    algorithm/merged_for_each.cpp
    algorithm/merged_none_of.cpp
    algorithm/narrow_integral.cpp
    algorithm/overload.cpp
    algorithm/pop_while.cpp
    algorithm/random_select.cpp
    algorithm/range.cpp
    algorithm/range_extended.cpp
    algorithm/range_step.cpp
    algorithm/round.cpp
    algorithm/shuffle.cpp
    algorithm/sort_pair.cpp
    algorithm/to_underlying.cpp
    algorithm/to_vector.cpp
    algorithm/transform_adjacent.cpp
    algorithm/transform_combine_while.cpp
    algorithm/transform_if.cpp
    algorithm/transform_to_container.cpp
    algorithm/transform_to_vector.cpp
    algorithm/u8_conversion.cpp
    algorithm/uniform_int_distribution.cpp

    allocated_size/ankerl_unordered_dense.cpp
    allocated_size/folly_small_vector.cpp
    allocated_size/std_optional.cpp
    allocated_size/std_pair.cpp
    allocated_size/std_string.cpp
    allocated_size/std_vector.cpp
    allocated_size/tracked_resource.cpp
    allocated_size/trait.cpp

    benchmark/render_line_scene.cpp
    benchmark/schematic_creation.cpp
    benchmark/simulation_runtime.cpp

    component/editable_circuit/editing/edit_logicitem.cpp
    component/editable_circuit/editing/edit_logicitem_detail.cpp
    component/editable_circuit/editing/edit_wire.cpp
    component/editable_circuit/editing/edit_wire_detail.cpp
    component/editable_circuit/circuit_data.cpp
    component/editable_circuit/layout_index.cpp
    component/editable_circuit/modifier.cpp
    component/editable_circuit/selection_guard.cpp
    component/editable_circuit/selection_store.cpp
    component/editable_circuit/visible_selection.cpp
    component/interactive_simulation/interaction_cache.cpp
    component/layout/logicitem_store.cpp
    component/layout/wire_store.cpp
    component/line_tree/line_store.cpp
    component/line_tree/tree_builder.cpp
    component/simulation/history_buffer.cpp
    component/simulation/history_calculation.cpp
    component/simulation/history_entry.cpp
    component/simulation/history_index.cpp
    component/simulation/history_iterator.cpp
    component/simulation/history_min_index.cpp
    component/simulation/history_view.cpp
    component/simulation/simulation_event.cpp
    component/simulation/simulation_event_group.cpp
    component/simulation/simulation_queue.cpp

    concept/explicitly_convertible.cpp
    concept/input_range.cpp
    concept/integral.cpp
    concept/member_format_function.cpp
    concept/range_value_type.cpp
    concept/string_view.cpp

    container/graph/visitor/calling_visitor.cpp
    container/graph/visitor/empty_visitor.cpp
    container/graph/visitor/length_recorder_visitor.cpp
    container/graph/visitor/printing_visitor.cpp
    container/graph/adjacency_graph.cpp
    container/graph/combine_visitor.cpp
    container/graph/depth_first_search.cpp
    container/graph/visitor_concept.cpp
    container/circular_buffer.cpp
    container/enum_map.cpp
    container/static_vector.cpp
    container/value_pointer.cpp

    format/blend2d_type.cpp
    format/container.cpp
    format/enum.cpp
    format/escape_ascii.cpp
    format/pointer.cpp
    format/std_type.cpp
    format/struct.cpp
    format/time.cpp
    format/to_hex.cpp

    geometry/connection.cpp
    geometry/connection_count.cpp
    geometry/display_state_map.cpp
    geometry/grid.cpp
    geometry/interpolation.cpp
    geometry/layout_calculation.cpp
    geometry/line.cpp
    geometry/offset.cpp
    geometry/orientation.cpp
    geometry/part.cpp
    geometry/part_selections.cpp
    geometry/point.cpp
    geometry/rect.cpp
    geometry/scene.cpp
    geometry/segment_info.cpp
    geometry/to_points_sorted_unique.cpp
    geometry/to_points_with_both_orientation.cpp

    index/collision_index.cpp
    index/connection_index.cpp
    index/segment_map.cpp
    index/spatial_index.cpp
    index/spatial_point_index.cpp

    iterator_adaptor/enumerate.cpp
    iterator_adaptor/filter_output_iterator.cpp
    iterator_adaptor/output_callable.cpp
    iterator_adaptor/polling_iterator.cpp
    iterator_adaptor/transform_if_output_iterator.cpp
    iterator_adaptor/transform_output_iterator.cpp
    iterator_adaptor/transform_view.cpp

    logic_item/layout.cpp
    logic_item/layout_display.cpp
    logic_item/layout_display_ascii.cpp
    logic_item/layout_display_number.cpp
    logic_item/layout_standard_element.cpp
    logic_item/schematic_info.cpp
    logic_item/simulation_info.cpp

    random/bool.cpp
    random/connection_count.cpp
    random/generator.cpp
    random/grid.cpp
    random/insertion_mode.cpp
    random/internal_state_count.cpp
    random/layout_calculation_data.cpp
    random/logicitem_type.cpp
    random/ordered_line.cpp
    random/orientation.cpp
    random/part.cpp
    random/point.cpp
    random/random_schematic.cpp
    random/segment.cpp
    random/wire.cpp

    render/primitive/arrow.cpp
    render/primitive/circle.cpp
    render/primitive/icon.cpp
    render/primitive/line.cpp
    render/primitive/point.cpp
    render/primitive/rect.cpp
    render/primitive/round_rect.cpp
    render/primitive/stroke.cpp
    render/primitive/text.cpp
    render/bl_error_check.cpp
    render/bl_glyph_placement.cpp
    render/context.cpp
    render/context_cache.cpp
    render/context_guard.cpp
    render/context_info.cpp
    render/font.cpp
    render/managed_context.cpp
    render/svg_cache.cpp
    render/text_alignment.cpp
    render/text_cache.cpp
    render/text_shaping.cpp

    type_trait/add_const_to_reference.cpp
    type_trait/const_iterator.cpp
    type_trait/is_equality_comparable.cpp
    type_trait/numeric_limits_template.cpp
    type_trait/safe_difference_type.cpp

    vocabulary/alignment.cpp
    vocabulary/circuit_id.cpp
    vocabulary/circuit_widget_state.cpp
    vocabulary/color.cpp
    vocabulary/connection.cpp
    vocabulary/connection_count.cpp
    vocabulary/connection_id.cpp
    vocabulary/connection_ids.cpp
    vocabulary/connector_info.cpp
    vocabulary/context_render_config.cpp
    vocabulary/default_mouse_action.cpp
    vocabulary/delay.cpp
    vocabulary/device_pixel_ratio.cpp
    vocabulary/direction_type.cpp
    vocabulary/display_state.cpp
    vocabulary/display_state_map.cpp
    vocabulary/drawable_element.cpp
    vocabulary/element_draw_state.cpp
    vocabulary/element_id.cpp
    vocabulary/element_type.cpp
    vocabulary/fallback_info.cpp
    vocabulary/font_style.cpp
    vocabulary/grid.cpp
    vocabulary/grid_fine.cpp
    vocabulary/insertion_mode.cpp
    vocabulary/internal_connection.cpp
    vocabulary/internal_connections.cpp
    vocabulary/internal_state.cpp
    vocabulary/internal_state_index.cpp
    vocabulary/layout_calculation_data.cpp
    vocabulary/layout_info_vector.cpp
    vocabulary/length.cpp
    vocabulary/length_vector.cpp
    vocabulary/line.cpp
    vocabulary/line_fine.cpp
    vocabulary/line_index.cpp
    vocabulary/line_insertion_type.cpp
    vocabulary/logic_small_vector.cpp
    vocabulary/logic_vector.cpp
    vocabulary/logicitem_connection.cpp
    vocabulary/logicitem_definition.cpp
    vocabulary/logicitem_id.cpp
    vocabulary/logicitem_type.cpp
    vocabulary/mouse_postion_info.cpp
    vocabulary/offset.cpp
    vocabulary/optional_logic_value.cpp
    vocabulary/optional_logic_values.cpp
    vocabulary/ordered_line.cpp
    vocabulary/orientation.cpp
    vocabulary/output_delays.cpp
    vocabulary/part.cpp
    vocabulary/part_copy_definition.cpp
    vocabulary/placed_element.cpp
    vocabulary/point.cpp
    vocabulary/point_device.cpp
    vocabulary/point_device_fine.cpp
    vocabulary/point_fine.cpp
    vocabulary/print_events.cpp
    vocabulary/realtime_timeout.cpp
    vocabulary/rect.cpp
    vocabulary/rect_fine.cpp
    vocabulary/render_mode.cpp
    vocabulary/segment.cpp
    vocabulary/segment_index.cpp
    vocabulary/segment_info.cpp
    vocabulary/segment_part.cpp
    vocabulary/segment_point_type.cpp
    vocabulary/selection_id.cpp
    vocabulary/setting_attribute.cpp
    vocabulary/shape_draw_type.cpp
    vocabulary/simulation_config.cpp
    vocabulary/size_device.cpp
    vocabulary/text_alignment.cpp
    vocabulary/time.cpp
    vocabulary/time_literal.cpp
    vocabulary/time_rate.cpp
    vocabulary/view_config.cpp
    vocabulary/widget_render_config.cpp
    vocabulary/wire_id.cpp

    base64.cpp
    circuit_description.cpp
    circuit_example.cpp
    copy_paste_clipboard.cpp
    default_element_definition.cpp
    editable_circuit.cpp
    event_counter.cpp
    event_counter_multi.cpp
    exception.cpp
    executable_path.cpp
    file.cpp
    font_style_property.cpp
    gzip.cpp
    interactive_simulation.cpp
    layout.cpp
    layout_info.cpp
    layout_message.cpp
    layout_message_forward.cpp
    layout_message_generation.cpp
    layout_message_validator.cpp
    line_tree.cpp
    line_tree_generation.cpp
    load_save_file.cpp
    logging.cpp
    part_selection.cpp
    render_caches.cpp
    render_circuit.cpp
    render_generic.cpp
    resource.cpp
    safe_numeric.cpp
    schematic.cpp
    schematic_generation.cpp
    segment_tree.cpp
    selection.cpp
    selection_normalization.cpp
    serialize.cpp
    serialize_detail.cpp
    setting_handle.cpp
    simulation.cpp
    simulation_player.cpp
    simulation_view.cpp
    size_handle.cpp
    spatial_simulation.cpp
    timeout_timer.cpp
    timer.cpp
    tree_normalization.cpp
    validate_definition.cpp
    vocabulary.cpp
    wyhash.cpp
)