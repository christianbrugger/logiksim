cmake_minimum_required(VERSION 3.25)

set(LS_CORE_SOURCES
    src/algorithm/accumulate.cpp
    src/algorithm/adjacent_count_if.cpp
    src/algorithm/all_equal.cpp
    src/algorithm/checked_deref.cpp
    src/algorithm/compare_sorted.cpp
    src/algorithm/contains.cpp
    src/algorithm/copy_adjacent_if.cpp
    src/algorithm/create_array.cpp
    src/algorithm/depth_first_visitor.cpp
    src/algorithm/distance_fast.cpp
    src/algorithm/fmt_join.cpp
    src/algorithm/has_duplicates_quadratic.cpp
    src/algorithm/make_unique.cpp
    src/algorithm/merged_any_of.cpp
    src/algorithm/merged_for_each.cpp
    src/algorithm/merged_none_of.cpp
    src/algorithm/narrow_integral.cpp
    src/algorithm/overload.cpp
    src/algorithm/pop_while.cpp
    src/algorithm/random_select.cpp
    src/algorithm/range.cpp
    src/algorithm/range_extended.cpp
    src/algorithm/range_step.cpp
    src/algorithm/round.cpp
    src/algorithm/shuffle.cpp
    src/algorithm/sort_pair.cpp
    src/algorithm/to_underlying.cpp
    src/algorithm/to_vector.cpp
    src/algorithm/transform_adjacent.cpp
    src/algorithm/transform_combine_while.cpp
    src/algorithm/transform_if.cpp
    src/algorithm/transform_to_container.cpp
    src/algorithm/transform_to_vector.cpp
    src/algorithm/u8_conversion.cpp
    src/algorithm/uniform_int_distribution.cpp

    src/allocated_size/ankerl_unordered_dense.cpp
    src/allocated_size/folly_small_vector.cpp
    src/allocated_size/std_optional.cpp
    src/allocated_size/std_pair.cpp
    src/allocated_size/std_string.cpp
    src/allocated_size/std_vector.cpp
    src/allocated_size/tracked_resource.cpp
    src/allocated_size/trait.cpp

    src/benchmark/render_line_scene.cpp
    src/benchmark/schematic_creation.cpp
    src/benchmark/simulation_runtime.cpp

    src/component/editable_circuit/editing/edit_logicitem.cpp
    src/component/editable_circuit/editing/edit_logicitem_detail.cpp
    src/component/editable_circuit/editing/edit_wire.cpp
    src/component/editable_circuit/editing/edit_wire_detail.cpp
    src/component/editable_circuit/circuit_data.cpp
    src/component/editable_circuit/layout_index.cpp
    src/component/editable_circuit/modifier.cpp
    src/component/editable_circuit/selection_guard.cpp
    src/component/editable_circuit/selection_store.cpp
    src/component/editable_circuit/visible_selection.cpp
    src/component/interactive_simulation/interaction_cache.cpp
    src/component/layout/logicitem_store.cpp
    src/component/layout/wire_store.cpp
    src/component/line_tree/line_store.cpp
    src/component/line_tree/tree_builder.cpp
    src/component/simulation/history_buffer.cpp
    src/component/simulation/history_calculation.cpp
    src/component/simulation/history_entry.cpp
    src/component/simulation/history_index.cpp
    src/component/simulation/history_iterator.cpp
    src/component/simulation/history_min_index.cpp
    src/component/simulation/history_view.cpp
    src/component/simulation/simulation_event.cpp
    src/component/simulation/simulation_event_group.cpp
    src/component/simulation/simulation_queue.cpp

    src/concept/explicitly_convertible.cpp
    src/concept/input_range.cpp
    src/concept/integral.cpp
    src/concept/member_format_function.cpp
    src/concept/range_value_type.cpp
    src/concept/string_view.cpp

    src/container/graph/visitor/calling_visitor.cpp
    src/container/graph/visitor/empty_visitor.cpp
    src/container/graph/visitor/length_recorder_visitor.cpp
    src/container/graph/visitor/printing_visitor.cpp
    src/container/graph/adjacency_graph.cpp
    src/container/graph/combine_visitor.cpp
    src/container/graph/depth_first_search.cpp
    src/container/graph/visitor_concept.cpp
    src/container/circular_buffer.cpp
    src/container/enum_map.cpp
    src/container/static_vector.cpp
    src/container/value_pointer.cpp

    src/format/blend2d_type.cpp
    src/format/container.cpp
    src/format/enum.cpp
    src/format/escape_ascii.cpp
    src/format/pointer.cpp
    src/format/std_type.cpp
    src/format/struct.cpp
    src/format/time.cpp
    src/format/to_hex.cpp

    src/geometry/connection.cpp
    src/geometry/connection_count.cpp
    src/geometry/display_state_map.cpp
    src/geometry/grid.cpp
    src/geometry/interpolation.cpp
    src/geometry/layout_calculation.cpp
    src/geometry/line.cpp
    src/geometry/offset.cpp
    src/geometry/orientation.cpp
    src/geometry/part.cpp
    src/geometry/part_selections.cpp
    src/geometry/point.cpp
    src/geometry/rect.cpp
    src/geometry/scene.cpp
    src/geometry/segment_info.cpp
    src/geometry/to_points_sorted_unique.cpp
    src/geometry/to_points_with_both_orientation.cpp

    src/index/collision_index.cpp
    src/index/connection_index.cpp
    src/index/segment_map.cpp
    src/index/spatial_index.cpp
    src/index/spatial_point_index.cpp

    src/iterator_adaptor/enumerate.cpp
    src/iterator_adaptor/filter_output_iterator.cpp
    src/iterator_adaptor/output_callable.cpp
    src/iterator_adaptor/polling_iterator.cpp
    src/iterator_adaptor/transform_if_output_iterator.cpp
    src/iterator_adaptor/transform_output_iterator.cpp
    src/iterator_adaptor/transform_view.cpp

    src/logic_item/layout.cpp
    src/logic_item/layout_display.cpp
    src/logic_item/layout_display_ascii.cpp
    src/logic_item/layout_display_number.cpp
    src/logic_item/layout_standard_element.cpp
    src/logic_item/schematic_info.cpp
    src/logic_item/simulation_info.cpp

    src/random/bool.cpp
    src/random/connection_count.cpp
    src/random/generator.cpp
    src/random/grid.cpp
    src/random/insertion_mode.cpp
    src/random/internal_state_count.cpp
    src/random/layout_calculation_data.cpp
    src/random/logicitem_type.cpp
    src/random/ordered_line.cpp
    src/random/orientation.cpp
    src/random/part.cpp
    src/random/point.cpp
    src/random/random_schematic.cpp
    src/random/segment.cpp
    src/random/wire.cpp

    src/render/primitive/arrow.cpp
    src/render/primitive/circle.cpp
    src/render/primitive/icon.cpp
    src/render/primitive/line.cpp
    src/render/primitive/point.cpp
    src/render/primitive/rect.cpp
    src/render/primitive/round_rect.cpp
    src/render/primitive/stroke.cpp
    src/render/primitive/text.cpp
    src/render/bl_error_check.cpp
    src/render/context.cpp
    src/render/context_cache.cpp
    src/render/context_guard.cpp
    src/render/context_info.cpp
    src/render/font.cpp
    src/render/managed_context.cpp
    src/render/svg_cache.cpp
    src/render/text_alignment.cpp
    src/render/text_cache.cpp
    src/render/text_shaping.cpp

    src/type_trait/add_const_to_reference.cpp
    src/type_trait/const_iterator.cpp
    src/type_trait/is_equality_comparable.cpp
    src/type_trait/numeric_limits_template.cpp
    src/type_trait/safe_difference_type.cpp

    src/vocabulary/alignment.cpp
    src/vocabulary/circuit_id.cpp
    src/vocabulary/circuit_widget_state.cpp
    src/vocabulary/color.cpp
    src/vocabulary/connection.cpp
    src/vocabulary/connection_count.cpp
    src/vocabulary/connection_id.cpp
    src/vocabulary/connection_ids.cpp
    src/vocabulary/connector_info.cpp
    src/vocabulary/context_render_config.cpp
    src/vocabulary/default_mouse_action.cpp
    src/vocabulary/delay.cpp
    src/vocabulary/device_pixel_ratio.cpp
    src/vocabulary/direction_type.cpp
    src/vocabulary/display_state.cpp
    src/vocabulary/display_state_map.cpp
    src/vocabulary/drawable_element.cpp
    src/vocabulary/element_draw_state.cpp
    src/vocabulary/element_id.cpp
    src/vocabulary/element_type.cpp
    src/vocabulary/fallback_info.cpp
    src/vocabulary/font_style.cpp
    src/vocabulary/grid.cpp
    src/vocabulary/grid_fine.cpp
    src/vocabulary/insertion_mode.cpp
    src/vocabulary/internal_connection.cpp
    src/vocabulary/internal_connections.cpp
    src/vocabulary/internal_state.cpp
    src/vocabulary/internal_state_index.cpp
    src/vocabulary/layout_calculation_data.cpp
    src/vocabulary/layout_info_vector.cpp
    src/vocabulary/length.cpp
    src/vocabulary/length_vector.cpp
    src/vocabulary/line.cpp
    src/vocabulary/line_fine.cpp
    src/vocabulary/line_index.cpp
    src/vocabulary/line_insertion_type.cpp
    src/vocabulary/logic_small_vector.cpp
    src/vocabulary/logic_vector.cpp
    src/vocabulary/logicitem_connection.cpp
    src/vocabulary/logicitem_definition.cpp
    src/vocabulary/logicitem_id.cpp
    src/vocabulary/logicitem_type.cpp
    src/vocabulary/mouse_postion_info.cpp
    src/vocabulary/offset.cpp
    src/vocabulary/optional_logic_value.cpp
    src/vocabulary/optional_logic_values.cpp
    src/vocabulary/ordered_line.cpp
    src/vocabulary/orientation.cpp
    src/vocabulary/output_delays.cpp
    src/vocabulary/part.cpp
    src/vocabulary/part_copy_definition.cpp
    src/vocabulary/placed_element.cpp
    src/vocabulary/point.cpp
    src/vocabulary/point_device.cpp
    src/vocabulary/point_device_fine.cpp
    src/vocabulary/point_fine.cpp
    src/vocabulary/print_events.cpp
    src/vocabulary/realtime_timeout.cpp
    src/vocabulary/rect.cpp
    src/vocabulary/rect_fine.cpp
    src/vocabulary/render_mode.cpp
    src/vocabulary/segment.cpp
    src/vocabulary/segment_index.cpp
    src/vocabulary/segment_info.cpp
    src/vocabulary/segment_part.cpp
    src/vocabulary/segment_point_type.cpp
    src/vocabulary/selection_id.cpp
    src/vocabulary/setting_attribute.cpp
    src/vocabulary/shape_draw_type.cpp
    src/vocabulary/simulation_config.cpp
    src/vocabulary/size_device.cpp
    src/vocabulary/text_alignment.cpp
    src/vocabulary/time.cpp
    src/vocabulary/time_literal.cpp
    src/vocabulary/time_rate.cpp
    src/vocabulary/view_config.cpp
    src/vocabulary/widget_render_config.cpp
    src/vocabulary/wire_id.cpp

    src/base64.cpp
    src/circuit_description.cpp
    src/circuit_example.cpp
    src/copy_paste_clipboard.cpp
    src/default_element_definition.cpp
    src/editable_circuit.cpp
    src/event_counter.cpp
    src/event_counter_multi.cpp
    src/exception.cpp
    src/executable_path.cpp
    src/file.cpp
    src/font_style_property.cpp
    src/gzip.cpp
    src/interactive_simulation.cpp
    src/layout.cpp
    src/layout_info.cpp
    src/layout_message.cpp
    src/layout_message_forward.cpp
    src/layout_message_generation.cpp
    src/layout_message_validator.cpp
    src/line_tree.cpp
    src/line_tree_generation.cpp
    src/load_save_file.cpp
    src/logging.cpp
    src/part_selection.cpp
    src/render_caches.cpp
    src/render_circuit.cpp
    src/render_generic.cpp
    src/resource.cpp
    src/safe_numeric.cpp
    src/schematic.cpp
    src/schematic_generation.cpp
    src/segment_tree.cpp
    src/selection.cpp
    src/selection_normalization.cpp
    src/serialize.cpp
    src/serialize_detail.cpp
    src/setting_handle.cpp
    src/simulation.cpp
    src/simulation_player.cpp
    src/simulation_view.cpp
    src/size_handle.cpp
    src/spatial_simulation.cpp
    src/timeout_timer.cpp
    src/timer.cpp
    src/tree_normalization.cpp
    src/validate_definition.cpp
    src/vocabulary.cpp
    src/wyhash.cpp
)

set(LS_GUI_SOURCES
    src/component/circuit_widget/mouse_logic/editing_logic_concept.cpp
    src/component/circuit_widget/mouse_logic/editing_logic_manager.cpp
    src/component/circuit_widget/mouse_logic/editing_logic_variant.cpp
    src/component/circuit_widget/mouse_logic/handle_resize.cpp
    src/component/circuit_widget/mouse_logic/handle_setting.cpp
    src/component/circuit_widget/mouse_logic/insert_logic_item.cpp
    src/component/circuit_widget/mouse_logic/insert_wire.cpp
    src/component/circuit_widget/mouse_logic/mouse_drag_logic.cpp
    src/component/circuit_widget/mouse_logic/mouse_wheel_logic.cpp
    src/component/circuit_widget/mouse_logic/selection_area.cpp
    src/component/circuit_widget/mouse_logic/selection_move.cpp
    src/component/circuit_widget/mouse_logic/selection_single.cpp
    src/component/circuit_widget/checked_editable_circuit.cpp
    src/component/circuit_widget/circuit_renderer.cpp
    src/component/circuit_widget/circuit_store.cpp
    src/component/circuit_widget/simulation_runner.cpp
    src/component/circuit_widget/zoom.cpp

    src/format/qt_type.cpp

    src/qt/clipboard_access.cpp
    src/qt/enum_to_string.cpp
    src/qt/mouse_position.cpp
    src/qt/mouse_position_p.cpp
    src/qt/path_conversion.cpp
    src/qt/point_conversion.cpp
    src/qt/setting_location.cpp
    src/qt/widget_geometry.cpp

    src/widget/circuit_widget.cpp
    src/widget/circuit_widget_base.cpp
    src/widget/render_widget.cpp
    src/widget/setting_dialog.cpp
    src/widget/setting_dialog_manager.cpp
    src/widget/top_widget.cpp
)

# any file that contains Q_OBJECT
# list them manually to speedup compilation
set(MOC_FILES
    src/widget/circuit_widget_base.h
    src/widget/render_widget.h
    src/widget/setting_dialog.h
    src/widget/setting_dialog_manager.h
    src/widget/top_widget.h
)

set(LS_TEST_SOURCES
    test/algorithm/fmt_join_test.cpp
    test/algorithm/range_extended_test.cpp
    test/algorithm/range_step_test.cpp
    test/algorithm/range_test.cpp
    test/algorithm/shuffle_test.cpp
    test/algorithm/transform_to_vector_test.cpp
    test/algorithm/uniform_int_distribution_test.cpp

    test/component/simulation/history_index_test.cpp
    test/component/simulation/history_view_test.cpp
    test/component/simulation/simulation_event_test.cpp

    test/container/graph/depth_first_search_test.cpp
    test/container/circular_buffer_test.cpp
    test/container/static_vector_test.cpp
    test/container/value_pointer_pimpl.cpp
    test/container/value_pointer_pimpl_test.cpp
    test/container/value_pointer_test.cpp

    test/editable_circuit/modifier/logic_item_test.cpp
    test/editable_circuit/modifier/modifier_config_test.cpp
    test/editable_circuit/modifier/wire_random_test.cpp
    test/editable_circuit/modifier/wire_test.cpp
    test/editable_circuit/editable_circuit_random_test.cpp

    test/format/container_test.cpp

    test/geometry/connection_count_test.cpp
    test/geometry/line_test.cpp
    test/geometry/part_selections.cpp
    test/geometry/part_test.cpp

    test/iterator_adaptor/enumerate_test.cpp
    test/iterator_adaptor/enumerate_transform_view_test.cpp
    test/iterator_adaptor/polling_iterator_test.cpp
    test/iterator_adaptor/transform_view_test.cpp

    test/logic_item/layout_test.cpp
    test/logic_item/schematic_info_test.cpp

    test/random/generator_test.cpp

    test/vocabulary/color_test.cpp
    test/vocabulary/connection_count_test.cpp
    test/vocabulary/connection_id_test.cpp
    test/vocabulary/delay_test.cpp
    test/vocabulary/element_id_test.cpp
    test/vocabulary/grid_fine_test.cpp
    test/vocabulary/grid_test.cpp
    test/vocabulary/line_fine_test.cpp
    test/vocabulary/line_test.cpp
    test/vocabulary/offset_test.cpp
    test/vocabulary/ordered_line_test.cpp
    test/vocabulary/part_test.cpp
    test/vocabulary/point_fine_test.cpp
    test/vocabulary/point_test.cpp
    test/vocabulary/rect_fine_test.cpp
    test/vocabulary/rect_test.cpp
    test/vocabulary/segment_index_test.cpp
    test/vocabulary/segment_test.cpp
    test/vocabulary/time_test.cpp

    test/layout_test.cpp
    test/line_tree_test.cpp
    test/part_selection_test.cpp
    test/schematic_test.cpp
    test/segment_tree_test.cpp
    test/simulation_test.cpp
    test/timer_test.cpp
)

cmake_policy(PUSH)

if(POLICY CMP0092)
  # Don't add -W3 warning level by default.
  cmake_policy(SET CMP0092 NEW)
endif()
if(POLICY CMP0141)
  # don't use 'MSVC_DEBUG_INFORMATION_FORMAT (BOOST bug 2022-12-14)  # TODO check 
  cmake_policy(SET CMP0141 OLD)
endif()
if(POLICY CMP0069)
  # enforce link time optimization is used when enabled
  cmake_policy(SET CMP0069 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
endif()

project(simulation_srp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
# set(CMAKE_CXX_EXTENSIONS OFF)




include(CMakePrintHelpers)
cmake_print_variables(CMAKE_C_COMPILER)
cmake_print_variables(CMAKE_CXX_COMPILER)

option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." ON)
option(LS_ENABLE_LTO "Enable link time optimization." OFF)
option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)
option(LS_ENABLE_COVERAGE "Enable coverage for tests under Clang" OFF)

# option for sanitizers
set(LS_SANITIZE "" CACHE STRING
    "Enable sanitizers (Options: Address Undefined Address;Undefined 
    Memory MemoryWithOrigins Thread). Defaults to empty string.")
set_property(
    CACHE LS_SANITIZE PROPERTY STRINGS 
    "" Address Undefined Address;Undefined Memory Thread
)

# option for libc++
set(LS_USE_LIBCXX AUTO CACHE STRING "Compile all dependencies against clang libc++.")
set_property(CACHE LS_USE_LIBCXX PROPERTY STRINGS AUTO ON OFF)

if ((LS_USE_LIBCXX STREQUAL "AUTO" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
       (LS_SANITIZE STREQUAL "Memory" OR  LS_SANITIZE STREQUAL "MemoryWithOrigins" OR 
        LS_SANITIZE STREQUAL "Thread")) 
    OR (NOT LS_USE_LIBCXX STREQUAL "AUTO" AND LS_USE_LIBCXX)
)
    set(LS_USE_LIBCXX_BOOL TRUE)
else()
    set(LS_USE_LIBCXX_BOOL FALSE)
endif()

#message("TEST LS_SANITIZE = ${LS_SANITIZE}")
#message("TEST LS_USE_LIBCXX = ${LS_USE_LIBCXX}")
#message("TEST LS_USE_LIBCXX_BOOL = ${LS_USE_LIBCXX_BOOL}")



# global exceptions
if (MSVC)
	add_compile_options(/EHsc)
else()
	add_compile_options(-fexceptions)
endif()


# global architecture
if (MSVC)
	# add_compile_options(/arch:AVX)
	add_compile_options(/arch:AVX2)
	# add_compile_options(/arch:AVX512)
else()
	add_compile_options(-march=native)
endif()

# global profiling
#if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# add_link_options(/PROFILE)
#endif()


set(MAIN_COMPILE_OPTIONS)

# utf-8
if (MSVC)
	list(APPEND MAIN_COMPILE_OPTIONS /utf-8)
else()
endif()

# our library: needs to be standard complient
if (MSVC)
    list(APPEND MAIN_COMPILE_OPTIONS /permissive-)
else()
    list(APPEND MAIN_COMPILE_OPTIONS -pedantic)
endif()

# our library: enable warnings
if (MSVC)
    list(APPEND MAIN_COMPILE_OPTIONS /WX /W4 /external:W0)
    # list(APPEND MAIN_COMPILE_OPTIONS /analyze /analyze:external-)
    
    # list(APPEND MAIN_COMPILE_OPTIONS /we4062 /we4826 /we5204 /we5219 /we5240)
    # list(APPEND MAIN_COMPILE_OPTIONS /we4242 /we4254 /we4287 /we4388)
    # list(APPEND MAIN_COMPILE_OPTIONS /we4263 /we4264 /we4265 /we4266 /we4355)
    # list(APPEND MAIN_COMPILE_OPTIONS /we4296 /we4437 /we4471 /we4545 /we4582 /we4583)
    list(APPEND MAIN_COMPILE_OPTIONS /we5263)

    # list(APPEND MAIN_COMPILE_OPTIONS /we4365)
    
    # disable Spectre mitigation code generation warnings in external files
    list(APPEND MAIN_COMPILE_OPTIONS /wd5045)
    # disable left-to-right evaluation order in braced initializer list
    # list(APPEND MAIN_COMPILE_OPTIONS /wd4868)
    

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        # unreachable code warnings when using folly::small_vector in release
        list(APPEND MAIN_COMPILE_OPTIONS /wd4702)
    else()
        # disable unused parameters in debug
        list(APPEND MAIN_COMPILE_OPTIONS /wd4100)
    endif()
else()
    # -Wthread-safety 
    list(APPEND MAIN_COMPILE_OPTIONS -W -Wall -Wextra -Wshadow -Werror 
                             -Wstrict-aliasing) # -Wconversion) # -Wsign-conversion)


    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
        (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
        # g++ generates those in library headers for sanitized builds
        list(APPEND MAIN_COMPILE_OPTIONS -Wno-maybe-uninitialized)
        # also happens in benchmark which uses Werror
        set(BENCHMARK_ENABLE_WERROR OFF)
    endif()
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # clang 16 warns on brace elision, why?
    list(APPEND MAIN_COMPILE_OPTIONS -Wno-missing-braces)
endif()

if (LS_ENABLE_LTO)
    message(NOTICE "LOGIKSIM: Enabling link time optimization.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(NOTICE "LOGIKSIM: Disabling link time optimization.")
endif()

# better debugging profile
if (CMAKE_BUILD_TYPE STREQUAL "Debug")  # AND LS_SANITIZE
    if (MSVC)
        # add_compile_options(/O1)
    else()
        add_compile_options(-O1 -fno-optimize-sibling-calls)
    endif()
endif()

if (LS_ENABLE_TIME_TRACE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(NOTICE "LOGIKSIM: Enabling time tracing.")
        list(APPEND MAIN_COMPILE_OPTIONS -ftime-trace)
    else()
        message(FATAL_ERROR "Time tracing only works for Clang!" )
    endif()
endif()



if (LS_ENABLE_CCACHE)
    message(NOTICE "LOGIKSIM: Enabling ccache.")
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    # ccache only supports /Z7, not /Zi
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
else()
    message(NOTICE "LOGIKSIM: Disabling ccache.")
endif()
cmake_print_variables(CMAKE_C_FLAGS_DEBUG)
cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
cmake_print_variables(CMAKE_C_FLAGS_RELWITHDEBINFO)
cmake_print_variables(CMAKE_CXX_FLAGS_RELWITHDEBINFO)


cmake_print_variables(MAIN_COMPILE_OPTIONS)





list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")


if (LS_USE_LIBCXX_BOOL)
    message(NOTICE "LOGIKSIM: Using clang libc++.")

    set(LLVM_ENABLE_RUNTIMES "libcxx;libcxxabi;libunwind" CACHE STRING "" FORCE)

    #set(LIBCXX_ENABLE_SHARED OFF)
    #set(LIBCXXABI_ENABLE_SHARED OFF)
    #set(LIBCXX_ENABLE_STATIC ON)
    #set(LIBCXXABI_ENABLE_STATIC ON)

    set(LIBCXX_ENABLE_SHARED ON)
    set(LIBCXXABI_ENABLE_SHARED ON)
    set(LIBCXX_ENABLE_STATIC OFF)
    set(LIBCXXABI_ENABLE_STATIC OFF)

    set(LLVM_USE_SANITIZER ${LS_SANITIZE})
    add_subdirectory(external/llvm-project/runtimes EXCLUDE_FROM_ALL SYSTEM)
    link_libraries(
        #cxx_static
        #cxxabi_static
        cxx_shared
        cxxabi_shared
        #cxx_experimental
    )

    # The google benchmark contains an unconditional export that requires
    # all its dependencies to be exportable. By itself llvm does not define
    # export for runtime targets. Thats why we define them here.
    export(TARGETS 
        cxx_shared cxxabi_shared cxx-headers cxxabi_shared_objects 
        libcxx-abi-headers cxxabi-headers 
        FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/LlvmRuntimeTargets.cmake"
    )
else()
    message(NOTICE "LOGIKSIM: Using default stdlib.")
endif()


# TODO -fsanitize=float-divide-by-zero

# Set Sanitizer flags
#
# Set these flags after adding clang libc++. For those we pass
# our sanitizer config and the generic settings don't work.
#
if (LS_SANITIZE STREQUAL "")
elseif(LS_SANITIZE STREQUAL "Address")
    add_compile_options(-fsanitize=address) 
    add_link_options(-fsanitize=address)
elseif(LS_SANITIZE STREQUAL "Undefined")
    add_compile_options(-fsanitize=undefined) 
    add_link_options(-fsanitize=undefined)
elseif(LS_SANITIZE STREQUAL "Address;Undefined")
    add_compile_options(-fsanitize=address -fsanitize=undefined) 
    add_link_options(-fsanitize=address -fsanitize=undefined)
elseif(LS_SANITIZE STREQUAL "Memory")
    add_compile_options(-fsanitize=memory)
    add_link_options(-fsanitize=memory)
elseif(LS_SANITIZE STREQUAL "MemoryWithOrigins")
    add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins) 
    add_link_options(-fsanitize=memory -fsanitize-memory-track-origins)
elseif(LS_SANITIZE STREQUAL "Thread")
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
    set(BLEND2D_NO_JIT ON)
else()
    message(FATAL_ERROR "Unknown LS_SANITIZE Option: ${LS_SANITIZE}")
endif()
if (LS_SANITIZE)
    add_compile_options(-fno-sanitize-recover=all -fno-omit-frame-pointer -g) 
    add_link_options(-fno-sanitize-recover=all)
endif()

# set(BLEND2D_NO_JIT ON)




###### Dependencies

# Google Benchmark
set(BENCHMARK_ENABLE_LTO OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
add_subdirectory(external/benchmark EXCLUDE_FROM_ALL SYSTEM)

# Google Test
set(INSTALL_GTEST OFF)
add_subdirectory(external/googletest EXCLUDE_FROM_ALL SYSTEM)

# Zlib
set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)

# Boost
set(BOOST_IOSTREAMS_ENABLE_ZLIB ON)
set(BOOST_INCLUDE_LIBRARIES 
    algorithm 
    container 
    geometry
    iostreams
    mpl 
    random 
    safe_numerics
    unordered
)
add_subdirectory(external/boost EXCLUDE_FROM_ALL SYSTEM)

# Qt 6
set(CMAKE_AUTOMOC FALSE)  # takes too much time, not parallelized
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
message(NOTICE "Qt version: ${Qt6_VERSION}")

QT6_WRAP_CPP(MOC_SOURCES ${MOC_FILES})

# Blend2D
set(BLEND2D_STATIC TRUE)
set(BLEND2D_NO_INSTALL TRUE)
# set(BLEND2D_NO_JIT ON)
add_subdirectory(external/blend2d EXCLUDE_FROM_ALL SYSTEM)

# fmt
set(FMT_INSTALL FALSE)
add_subdirectory(external/fmt EXCLUDE_FROM_ALL SYSTEM)

# range-v3
add_subdirectory(external/range-v3 EXCLUDE_FROM_ALL SYSTEM)

# GSL
add_subdirectory(external/GSL EXCLUDE_FROM_ALL SYSTEM)

# abseil
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(external/abseil-cpp EXCLUDE_FROM_ALL SYSTEM)

# Folly - we build a minimal version of the library, only what we need
set(FOLLY_DIR_NAME my_folly)
add_library(folly
    external/${FOLLY_DIR_NAME}/folly/memory/detail/MallocImpl.cpp
    external/${FOLLY_DIR_NAME}/folly/lang/SafeAssert.cpp
    external/${FOLLY_DIR_NAME}/folly/ScopeGuard.cpp
    external/${FOLLY_DIR_NAME}/folly/lang/ToAscii.cpp
)
target_include_directories(folly SYSTEM PUBLIC 
    external/${FOLLY_DIR_NAME}
)
target_compile_definitions(folly PUBLIC FOLLY_NO_CONFIG)

# unordered_dense (ankerl)
add_subdirectory(external/unordered_dense EXCLUDE_FROM_ALL SYSTEM)

# glaze
add_subdirectory(external/glaze EXCLUDE_FROM_ALL SYSTEM)

# cppcodec / base64
add_subdirectory(external/cppcodec EXCLUDE_FROM_ALL SYSTEM)

# harfbuzz
set(SKIP_INSTALL_ALL TRUE)
add_subdirectory(external/harfbuzz EXCLUDE_FROM_ALL SYSTEM)

# GCE-Math
add_subdirectory(external/gcem EXCLUDE_FROM_ALL SYSTEM)

# svg2b2d
add_subdirectory(external/svg2b2d EXCLUDE_FROM_ALL SYSTEM)

# expected
set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(external/expected EXCLUDE_FROM_ALL SYSTEM)

# whereami - has no cmake
add_library(whereami STATIC EXCLUDE_FROM_ALL
    external/whereami/src/whereami.c
)
set_source_files_properties(
    external/whereami/src/whereami.c
    PROPERTIES LANGUAGE CXX
)
target_include_directories(whereami PUBLIC
    external/whereami/src
)


###################################################
# needed to run clang-tidy outside, generates compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (false)
    set(CMAKE_CXX_CLANG_TIDY
        clang-tidy;
        -extra-arg=/permissive-;
        -extra-arg=/EHsc;
        -warnings-as-errors=*;
        -checks=${CLANG_TIDY_CHECKS};
    )
    #-header-filter=\\src\\[a-z0-9_]+\.h$;
endif()





# Main Library

add_library(logicsim_pch INTERFACE)
if (LS_ENABLE_PCH)
    message(NOTICE "LOGIKSIM: Enabling pre-compiled headers.")

    target_precompile_headers(logicsim_pch INTERFACE
        <ankerl/unordered_dense.h>
        <blend2d.h>
        <boost/algorithm/string/join.hpp>
        <boost/container/small_vector.hpp>
        <boost/container/vector.hpp>
        <boost/geometry.hpp>
        <boost/random/mersenne_twister.hpp>
        <boost/random/uniform_int_distribution.hpp>
        <boost/safe_numerics/safe_integer.hpp>
        <fmt/core.h>
        <fmt/format.h>
        <folly/small_vector.h>
        <gcem.hpp>
        <gsl/gsl>
    
        <algorithm>
        <chrono>
        <cmath>
        <concepts>
        <cstdint>
        <exception>
        <filesystem>
        <functional>
        <iterator>
        <locale>
        <optional>
        <ostream>
        <queue>
        <random>
        <ranges>
        <ratio>
        <string>
        <string_view>
        <type_traits>
        <utility>
        <variant>
        <vector>
    )

    # this is needed so ccache works with PCH files, see:
    # https://ccache.dev/manual/latest.html#_precompiled_headers
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(logicsim_pch INTERFACE
            -Xclang -fno-pch-timestamp
        )
    endif()
else()
    message(NOTICE "LOGIKSIM: Disabling pre-compiled headers.")
endif()

set(LS_CORE_INCLUDE_DIRS
    src
)
set(LS_CORE_LIBRARIES
    Boost::algorithm
    Boost::container
    Boost::geometry
    Boost::iostreams
    Boost::mpl
    Boost::random
	Boost::safe_numerics
    Boost::unordered
    
    cppcodec
    fmt::fmt
    folly
    absl::base
    absl::flat_hash_map
    unordered_dense::unordered_dense
    GSL
    range-v3
    Blend2D::Blend2D
    glaze::glaze
    ZLIB::ZLIB
    harfbuzz
    gcem
    svg2b2d
    tl::expected
    whereami
)

# Resources
file(GLOB_RECURSE
    LS_RESOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/resources/**.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/**.svg
)
FOREACH(LS_RESOURCE_FILE ${LS_RESOURCE_FILES})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LS_RESOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${LS_RESOURCE_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${LS_RESOURCE_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LS_RESOURCE_FILE}
  )
ENDFOREACH()
list(TRANSFORM LS_RESOURCE_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

# MAIN LIBRARY

set(MAIN_LIBRARY "simulation_srp")
add_library(${MAIN_LIBRARY} 
    ${LS_CORE_SOURCES}
    ${LS_RESOURCE_FILES}
)
target_include_directories(${MAIN_LIBRARY} PUBLIC
    ${LS_CORE_INCLUDE_DIRS}
)
target_link_libraries(${MAIN_LIBRARY} PUBLIC 
    ${LS_CORE_LIBRARIES}
)
target_link_libraries(${MAIN_LIBRARY} PRIVATE 
    logicsim_pch
)
target_compile_options(${MAIN_LIBRARY} PUBLIC ${MAIN_COMPILE_OPTIONS})


# MAIN

set(MAIN_TARGET "simulation_srp_main")
add_executable(${MAIN_TARGET}
    src/main.cpp
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    ${MAIN_LIBRARY} 
)
set_property(TARGET ${MAIN_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

# MAIN GUI

set(MAIN_GUI_TARGET "simulation_srp_gui")
add_executable(${MAIN_GUI_TARGET}
    ${LS_GUI_SOURCES}
    ${MOC_SOURCES}

    src/main_gui.cpp
)
target_link_libraries(${MAIN_GUI_TARGET} PRIVATE 
    ${MAIN_LIBRARY}
    
    Qt6::Core
    Qt6::Widgets
    Qt6::GuiPrivate
)
set_property(TARGET ${MAIN_GUI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${MAIN_GUI_TARGET} PRIVATE 
        LS_EXCEPTION_MESSAGE_BOX
    )
    if (WIN32)
        # set_property(TARGET ${MAIN_GUI_TARGET} PROPERTY WIN32_EXECUTABLE true)
        # target_compile_definitions(${MAIN_GUI_TARGET} PRIVATE LS_LOG_TO_FILE)
    endif ()
endif ()


# MAIN BENCHMARK

set(MAIN_BENCHMARK_TARGET "simulation_srp_benchmark")
add_executable(${MAIN_BENCHMARK_TARGET}
    src/main_bechmark.cpp
)
target_link_libraries(${MAIN_BENCHMARK_TARGET} PRIVATE 
    ${MAIN_LIBRARY}
    benchmark::benchmark
)
set_property(TARGET ${MAIN_BENCHMARK_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


# Google Test

set(MAIN_TEST_TARGET "simulation_srp_test")
add_executable(${MAIN_TEST_TARGET}
    ${LS_TEST_SOURCES}
)
target_link_libraries(${MAIN_TEST_TARGET} PRIVATE 
    ${MAIN_LIBRARY} 
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
)
target_include_directories(${MAIN_TEST_TARGET} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
set_property(TARGET ${MAIN_TEST_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

if (LS_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")  #  AND CMAKE_BUILD_TYPE MATCHES Debug
        message(NOTICE "LOGIKSIM: Enabling Clang Coverage for Tests.")
        target_compile_options(${MAIN_TEST_TARGET} PRIVATE 
            -fprofile-instr-generate 
            -fcoverage-mapping
        )
        target_link_options(${MAIN_TEST_TARGET} PRIVATE 
            -fprofile-instr-generate 
            -fcoverage-mapping
        )
    else()
        message(FATAL_ERROR "Coverage only works for Clang!" )
    endif()
endif()


enable_testing()
include(GoogleTest)
gtest_discover_tests(${MAIN_TEST_TARGET})






# end of file
cmake_policy(POP)
