
#include "core/algorithm/overload.h"
#include "core/algorithm/range.h"
#include "core/algorithm/span_operations.h"
#include "core/algorithm/text_escape.h"
#include "core/component/editable_circuit/key_state.h"
#include "core/component/editable_circuit/modifier.h"
#include "core/geometry/layout_geometry.h"
#include "core/geometry/rect.h"
#include "core/layout_info.h"
#include "core/logging.h"
#include "core/random/fuzz.h"
#include "core/selection_sanitization.h"

#include <cstdint>
#include <span>

namespace logicsim {

namespace editable_circuit {

namespace {

struct FuzzLimits {
    rect_t box {};

    [[nodiscard]] auto operator==(const FuzzLimits&) const -> bool = default;
};

[[nodiscard]] auto all_within_limits(const Layout& layout, FuzzLimits limits) -> bool {
    return enclosing_rect(bounding_rect(layout), limits.box) == limits.box;
}

auto fuzz_select_insertion_mode(FuzzStream& stream) -> InsertionMode {
    switch (fuzz_small_int(stream, 0, 2)) {
        case 0:
            return InsertionMode::insert_or_discard;
        case 1:
            return InsertionMode::collisions;
        case 2:
            return InsertionMode::temporary;
    }
    std::terminate();
}

auto fuzz_select_insertion_hint(FuzzStream& stream,
                                InsertionMode new_mode) -> InsertionHint {
    if (new_mode == InsertionMode::temporary) {
        return InsertionHint::no_hint;
    }

    switch (fuzz_small_int(stream, 0, 1)) {
        case 0:
            return InsertionHint::no_hint;
        case 1:
            return InsertionHint::assume_colliding;
    }
    std::terminate();
}

auto fuzz_select_temporary_segment(FuzzStream& stream, Modifier& modifier) -> segment_t {
    const auto temporary_count =
        get_temporary_segment_count(modifier.circuit_data().layout);

    if (temporary_count == 0) {
        return null_segment;
    }

    const auto max_segment_index = clamp_to_fuzz_stream(temporary_count - 1);
    return segment_t {
        temporary_wire_id,
        segment_index_t {fuzz_small_int(stream, 0, max_segment_index)},
    };
}

auto fuzz_select_uninserted_segment(FuzzStream& stream, Modifier& modifier) -> segment_t {
    const auto temporary_count =
        get_temporary_segment_count(modifier.circuit_data().layout);
    const auto uninserted_count =
        temporary_count + get_colliding_segment_count(modifier.circuit_data().layout);

    if (uninserted_count == 0) {
        return null_segment;
    }

    const auto max_segment_index = clamp_to_fuzz_stream(uninserted_count - 1);
    const auto segment_int = fuzz_small_int(stream, 0, max_segment_index);

    if (std::cmp_less(segment_int, temporary_count)) {
        return segment_t {
            temporary_wire_id,
            segment_index_t {segment_int},
        };
    }
    return segment_t {
        colliding_wire_id,
        segment_index_t {segment_int - gsl::narrow_cast<int>(temporary_count)},
    };
}

auto fuzz_select_segment(FuzzStream& stream, Modifier& modifier) -> segment_t {
    const auto& segments = modifier.circuit_data().index.key_index().segments();

    if (segments.empty()) {
        return null_segment;
    }

    const auto max_index = clamp_to_fuzz_stream(segments.size() - 1);
    const auto index = fuzz_small_int(stream, 0, max_index);
    return checked_at(segments, index).first;
}

auto fuzz_select_part(FuzzStream& stream, Modifier& modifier,
                      segment_t segment) -> part_t {
    const auto part_full = get_part(modifier.circuit_data().layout, segment);
    const auto max_offset = clamp_to_fuzz_stream(part_full.end.value);
    const auto a = fuzz_small_int(stream, 0, max_offset - 1);
    const auto b = fuzz_small_int(stream, a + 1, max_offset);
    return part_t {a, b};
}

template <class Filter>
    requires std::invocable<Filter, const Layout&, logicitem_id_t> &&
                 std::same_as<bool,
                              std::invoke_result_t<Filter, const Layout&, logicitem_id_t>>
auto fuzz_select_logicitem_filter(FuzzStream& stream, Modifier& modifier,
                                  Filter filter) -> logicitem_id_t {
    const auto& layout = modifier.circuit_data().layout;

    const auto is_filtered = [&](logicitem_id_t logicitem_id) -> bool {
        return std::invoke(filter, layout, logicitem_id);
    };

    const auto count =
        gsl::narrow<int64_t>(std::ranges::count_if(logicitem_ids(layout), is_filtered));

    if (count == 0) {
        return null_logicitem_id;
    }

    const auto max_index = clamp_to_fuzz_stream(count - 1);
    const auto index = fuzz_small_int(stream, 0, max_index);

    auto view = logicitem_ids(layout) |                    //
                std::ranges::views::filter(is_filtered) |  //
                std::ranges::views::drop(index);

    Expects(view.begin() != view.end());
    return *view.begin();
}

auto fuzz_select_temporary_logicitem(FuzzStream& stream,
                                     Modifier& modifier) -> logicitem_id_t {
    const auto is_temporary = [](const Layout& layout, logicitem_id_t logicitem_id) {
        return layout.logicitems().display_state(logicitem_id) ==
               display_state_t::temporary;
    };

    return fuzz_select_logicitem_filter(stream, modifier, is_temporary);
}

auto fuzz_select_logicitem_type(FuzzStream& stream, Modifier& modifier,
                                LogicItemType type) -> logicitem_id_t {
    const auto is_type = [=](const Layout& layout, logicitem_id_t logicitem_id) {
        return layout.logicitems().type(logicitem_id) == type;
    };

    const auto result = fuzz_select_logicitem_filter(stream, modifier, is_type);

    Ensures(!result || is_type(modifier.circuit_data().layout, result));
    return result;
}

auto fuzz_select_temporary_decoration(FuzzStream& stream,
                                      Modifier& modifier) -> decoration_id_t {
    const auto& layout = modifier.circuit_data().layout;

    const auto is_temporary = [&](decoration_id_t decoration_id) {
        return layout.decorations().display_state(decoration_id) ==
               display_state_t::temporary;
    };

    const auto count =
        gsl::narrow<int64_t>(std::ranges::count_if(decoration_ids(layout), is_temporary));

    if (count == 0) {
        return null_decoration_id;
    }

    const auto max_index = clamp_to_fuzz_stream(count - 1);
    const auto index = fuzz_small_int(stream, 0, max_index);

    auto view = decoration_ids(layout) |  //
                std::ranges::views::filter(is_temporary) |
                std::ranges::views::drop(index);

    Expects(view.begin() != view.end());
    return *view.begin();
}

auto fuzz_select_logicitem(FuzzStream& stream, Modifier& modifier) -> logicitem_id_t {
    const auto& layout = modifier.circuit_data().layout;

    const auto size = layout.logicitems().size();

    if (size == std::size_t {0}) {
        return null_logicitem_id;
    }

    const auto max_index = clamp_to_fuzz_stream(size - std::size_t {1});
    return logicitem_id_t {fuzz_small_int(stream, 0, max_index)};
}

auto fuzz_select_decoration(FuzzStream& stream, Modifier& modifier) -> decoration_id_t {
    const auto& layout = modifier.circuit_data().layout;

    const auto size = layout.decorations().size();

    if (size == std::size_t {0}) {
        return null_decoration_id;
    }

    const auto max_index = clamp_to_fuzz_stream(size - std::size_t {1});
    return decoration_id_t {fuzz_small_int(stream, 0, max_index)};
}

auto fuzz_select_element(FuzzStream& stream, Modifier& modifier)
    -> std::variant<std::monostate, logicitem_id_t, segment_part_t, decoration_id_t> {
    const auto& layout = modifier.circuit_data().layout;

    const auto logicitem_count = layout.logicitems().size();
    const auto decoration_count = layout.decorations().size();
    const auto segment_count = get_segment_count(layout);

    const auto total_count = logicitem_count + decoration_count + segment_count;

    if (total_count == 0) {
        return std::monostate {};
    }

    const auto max_index = clamp_to_fuzz_stream(total_count - 1);
    const auto index = fuzz_small_int(stream, 0, max_index);

    if (std::cmp_less(index, logicitem_count)) {
        return logicitem_id_t {index};
    }
    if (std::cmp_less(index, logicitem_count + decoration_count)) {
        return decoration_id_t {index - logicitem_count};
    }

    const auto& segments = modifier.circuit_data().index.key_index().segments();
    const auto segment =
        checked_at(segments, index - logicitem_count - decoration_count).first;
    return segment_part_t {
        segment,
        fuzz_select_part(stream, modifier, segment),
    };
}

auto fuzz_select_selection(FuzzStream& stream, Modifier& modifier,
                           int max_count) -> Selection {
    const auto count = fuzz_small_int(stream, 0, max_count);

    auto selection = Selection {};
    for (const auto _ [[maybe_unused]] : range(count)) {
        const auto element = fuzz_select_element(stream, modifier);

        std::visit(overload([](std::monostate) {},
                            [&](logicitem_id_t logicitem_id) {
                                selection.add_logicitem(logicitem_id);
                            },
                            [&](decoration_id_t decoration_id) {
                                selection.add_decoration(decoration_id);
                            },
                            [&](segment_part_t segment_part) {
                                selection.add_segment(segment_part);
                            }),
                   element);
    }

    Ensures(std::cmp_less_equal(selection.size(), max_count));
    return selection;
}

auto fuzz_select_selection_function(FuzzStream& stream) -> SelectionFunction {
    switch (fuzz_small_int(stream, 0, 1)) {
        case 0:
            return SelectionFunction::add;
        case 1:
            return SelectionFunction::substract;
    };
    std::terminate();
}

auto fuzz_select_temporary_selection_full_parts(FuzzStream& stream, Modifier& modifier,
                                                int max_count) -> Selection {
    const auto count = fuzz_small_int(stream, 0, max_count);

    auto selection = Selection {};
    for (const auto _ [[maybe_unused]] : range(count)) {
        if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
            const auto part = get_part(modifier.circuit_data().layout, segment);
            selection.add_segment(segment_part_t {segment, part});
        }
    }

    Ensures(std::cmp_less_equal(selection.size(), max_count));
    return selection;
}

auto fuzz_select_move_delta(FuzzStream& stream, rect_t rect,
                            const FuzzLimits& limits) -> move_delta_t {
    return move_delta_t {
        .x = fuzz_small_int(stream,  //
                            int {limits.box.p0.x} - int {rect.p0.x},
                            int {limits.box.p1.x} - int {rect.p1.x}),
        .y = fuzz_small_int(stream,  //
                            int {limits.box.p0.y} - int {rect.p0.y},
                            int {limits.box.p1.y} - int {rect.p1.y}),
    };
}

auto fuzz_select_move_delta(FuzzStream& stream, ordered_line_t line,
                            const FuzzLimits& limits) -> move_delta_t {
    const auto rect = element_bounding_rect(line);
    return fuzz_select_move_delta(stream, rect, limits);
}

auto fuzz_select_move_delta(FuzzStream& stream, const layout_calculation_data_t& data,
                            const FuzzLimits& limits) -> move_delta_t {
    const auto rect = element_bounding_rect(data);
    return fuzz_select_move_delta(stream, rect, limits);
}

auto fuzz_select_move_delta(FuzzStream& stream, const decoration_layout_data_t& data,
                            const FuzzLimits& limits) -> move_delta_t {
    const auto rect = element_bounding_rect(data);
    return fuzz_select_move_delta(stream, rect, limits);
}

auto fuzz_select_point(FuzzStream& stream, const rect_t& limits) -> point_t {
    return point_t {
        grid_t {fuzz_small_int(stream, int {limits.p0.x}, int {limits.p1.x})},
        grid_t {fuzz_small_int(stream, int {limits.p0.y}, int {limits.p1.y})},
    };
}

auto fuzz_select_point(FuzzStream& stream, const FuzzLimits& limits) -> point_t {
    return fuzz_select_point(stream, limits.box);
}

auto fuzz_select_point_fine(FuzzStream& stream,
                            const rect_fine_t& limits) -> point_fine_t {
    return point_fine_t {
        grid_fine_t {
            fuzz_double_inclusive(stream, double {limits.p0.x}, double {limits.p1.x})},
        grid_fine_t {
            fuzz_double_inclusive(stream, double {limits.p0.y}, double {limits.p1.y})},
    };
}

auto fuzz_select_point_fine(FuzzStream& stream,
                            const FuzzLimits& limits) -> point_fine_t {
    return fuzz_select_point_fine(stream, rect_fine_t {limits.box});
}

[[maybe_unused]] auto fuzz_select_rect(FuzzStream& stream,
                                       const FuzzLimits& limits) -> rect_t {
    const auto p0 = fuzz_select_point(stream, limits);
    const auto p1 = fuzz_select_point(stream, rect_t {p0, limits.box.p1});

    return rect_t {p0, p1};
}

auto fuzz_select_rect_fine(FuzzStream& stream, const FuzzLimits& limits) -> rect_fine_t {
    const auto p0 = fuzz_select_point_fine(stream, limits);
    const auto p1 = fuzz_select_point_fine(stream, rect_fine_t {p0, limits.box.p1});

    return rect_fine_t {p0, p1};
}

auto fuzz_select_points(FuzzStream& stream, const FuzzLimits& limits, int min_count,
                        int max_count) {
    const auto count = fuzz_small_int(stream, min_count, max_count);
    const auto gen_point = [&]() -> point_t { return fuzz_select_point(stream, limits); };

    auto points = std::vector<point_t> {};
    std::ranges::generate_n(std::back_inserter(points), count, gen_point);

    Ensures(std::cmp_less_equal(min_count, points.size()) &&
            std::cmp_less_equal(points.size(), max_count));
    return points;
};

auto fuzz_select_shadow_or_crosspoint(FuzzStream& stream) -> SegmentPointType {
    return fuzz_bool(stream) ? SegmentPointType::shadow_point
                             : SegmentPointType::cross_point;
}

auto fuzz_select_non_taken_key(FuzzStream& stream, const KeyIndex& key_index,
                               int range = 15) -> segment_key_t {
    const auto to_segment_key = [](int64_t i) { return segment_key_t {i}; };
    const auto is_not_taken = [&key_index](const segment_key_t key) {
        return !key_index.contains(key);
    };

    const auto index = fuzz_small_int(stream, 0, range);

    auto key = std::ranges::views::iota(int64_t {0}) |
               std::ranges::views::transform(to_segment_key) |
               std::ranges::views::filter(is_not_taken) |  //
               std::ranges::views::drop(index);

    Expects(key.begin() != key.end());
    const auto result = *key.begin();

    Ensures(!key_index.contains(result));
    return result;
}

auto add_wire_segment(FuzzStream& stream, Modifier& modifier,
                      const FuzzLimits& limits) -> void {
    Expects(limits.box.p0.x < limits.box.p1.x);
    Expects(limits.box.p0.y < limits.box.p1.y);

    const bool horizontal = fuzz_bool(stream);

    const auto line = [&]() {
        if (horizontal) {
            const auto x0 =
                fuzz_small_int(stream, int {limits.box.p0.x}, int {limits.box.p1.x} - 1);
            const auto x1 = fuzz_small_int(stream, x0 + 1, int {limits.box.p1.x});
            const auto y =
                fuzz_small_int(stream, int {limits.box.p0.y}, int {limits.box.p1.y});

            return ordered_line_t {point_t {x0, y}, point_t {x1, y}};
        }
        const auto x =
            fuzz_small_int(stream, int {limits.box.p0.x}, int {limits.box.p1.x});
        const auto y0 =
            fuzz_small_int(stream, int {limits.box.p0.y}, int {limits.box.p1.y} - 1);
        const auto y1 = fuzz_small_int(stream, y0 + 1, int {limits.box.p1.y});

        return ordered_line_t {point_t {x, y0}, point_t {x, y1}};
    }();

    const auto mode = fuzz_select_insertion_mode(stream);

    modifier.add_wire_segment(line, mode);
}

auto delete_temporary_wire_segment(FuzzStream& stream, Modifier& modifier) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto part = fuzz_select_part(stream, modifier, segment);

        auto segment_part = segment_part_t {segment, part};
        modifier.delete_temporary_wire_segment(segment_part);
    }
}

auto change_wire_insertion_mode(FuzzStream& stream, Modifier& modifier) -> void {
    if (const auto segment = fuzz_select_segment(stream, modifier)) {
        const auto part = fuzz_select_part(stream, modifier, segment);
        const auto new_mode = fuzz_select_insertion_mode(stream);

        auto segment_part = segment_part_t {segment, part};

        if (change_wire_insertion_mode_requires_sanitization(segment_part, new_mode)) {
            const auto sanitize_mode =
                fuzz_bool(stream) ? SanitizeMode::expand : SanitizeMode::shrink;
            segment_part = sanitize_part(segment_part, modifier, sanitize_mode);
        }

        if (segment_part) {
            const auto hint = fuzz_select_insertion_hint(stream, new_mode);
            modifier.change_wire_insertion_mode(segment_part, new_mode, hint);
        }
    }
}

auto move_temporary_wire_unchecked(FuzzStream& stream, Modifier& modifier,
                                   const FuzzLimits& limits) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto segment_part =
            get_segment_part(modifier.circuit_data().layout, segment);

        const auto line = get_line(modifier.circuit_data().layout, segment);
        const auto delta = fuzz_select_move_delta(stream, line, limits);

        modifier.move_temporary_wire_unchecked(segment_part, delta);
    }
}

auto move_or_delete_temporary_wire(FuzzStream& stream, Modifier& modifier,
                                   const FuzzLimits& limits) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto part = fuzz_select_part(stream, modifier, segment);
        auto segment_part = segment_part_t {segment, part};

        const auto line = get_line(modifier.circuit_data().layout, segment_part);
        const auto delta = fuzz_select_move_delta(stream, line, limits);

        modifier.move_or_delete_temporary_wire(segment_part, delta);
    }
}

auto toggle_wire_crosspoint(FuzzStream& stream, Modifier& modifier,
                            const FuzzLimits& limits) -> void {
    const auto point = fuzz_select_point(stream, limits);
    modifier.toggle_wire_crosspoint(point);
}

auto set_temporary_endpoints(FuzzStream& stream, Modifier& modifier) -> void {
    if (const auto segment = fuzz_select_temporary_segment(stream, modifier)) {
        const auto endpoints = endpoints_t {
            .p0_type = fuzz_select_shadow_or_crosspoint(stream),
            .p1_type = fuzz_select_shadow_or_crosspoint(stream),
        };
        modifier.set_temporary_endpoints(segment, endpoints);
    }
}

auto merge_uninserted_segment(FuzzStream& stream, Modifier& modifier) -> void {
    const auto segment1 = fuzz_select_uninserted_segment(stream, modifier);
    const auto segment2 = fuzz_select_uninserted_segment(stream, modifier);

    if (are_uninserted_segments_mergeable(modifier, segment1, segment2)) {
        modifier.merge_uninserted_segment(segment1, segment2);
    }
}

auto split_uninserted_segment(FuzzStream& stream, Modifier& modifier) -> void {
    if (const auto segment = fuzz_select_uninserted_segment(stream, modifier)) {
        const auto full_part = get_part(modifier.circuit_data().layout, segment);
        const auto size = distance(full_part);

        if (size <= 1) {
            return;
        }
        const auto offset = offset_t {
            fuzz_small_int(stream, 1, clamp_to_fuzz_stream(size - 1)),
        };

        const auto new_key = [&stream, &modifier]() {
            if (fuzz_bool(stream)) {
                return fuzz_select_non_taken_key(
                    stream, modifier.circuit_data().index.key_index());
            }
            return null_segment_key;
        }();

        modifier.split_uninserted_segment(segment, offset, new_key);
    }
}

auto regularize_temporary_selection(FuzzStream& stream, Modifier& modifier,
                                    const FuzzLimits& limits) -> void {
    const auto guard = ModifierSelectionGuard {

        modifier,
        fuzz_select_temporary_selection_full_parts(stream, modifier, 4),
    };

    const auto& selection =
        modifier.circuit_data().selection_store.at(guard.selection_id());

    auto true_cross_points =
        fuzz_bool(stream) ? std::make_optional(fuzz_select_points(stream, limits, 0, 4))
                          : std::nullopt;

    modifier.regularize_temporary_selection(selection, std::move(true_cross_points));
}

auto split_temporary_segments(FuzzStream& stream, Modifier& modifier,
                              const FuzzLimits& limits) -> void {
    const auto split_points = fuzz_select_points(stream, limits, 0, 4);
    const auto selection =
        fuzz_select_temporary_selection_full_parts(stream, modifier, 4);

    modifier.split_temporary_segments(selection, split_points);
}

auto get_inserted_cross_points(FuzzStream& stream, Modifier& modifier) -> void {
    const auto selection1 = fuzz_select_selection(stream, modifier, 4);

    const auto split_points =
        editable_circuit::get_inserted_cross_points(modifier, selection1);

    const auto selection2 =
        fuzz_select_temporary_selection_full_parts(stream, modifier, 4);

    modifier.split_temporary_segments(selection2, split_points);
}

auto get_temporary_selection_splitpoints(FuzzStream& stream, Modifier& modifier) -> void {
    const auto selection =
        fuzz_select_temporary_selection_full_parts(stream, modifier, 4);

    const auto split_points =
        editable_circuit::get_temporary_selection_splitpoints(modifier, selection);

    modifier.split_temporary_segments(selection, split_points);
}

//
// Logic Item
//

auto delete_temporary_logicitem(FuzzStream& stream, Modifier& modifier) {
    if (auto logicitem_id = fuzz_select_temporary_logicitem(stream, modifier)) {
        modifier.delete_temporary_logicitem(logicitem_id);
    }
}

auto move_or_delete_temporary_logicitem(FuzzStream& stream, Modifier& modifier,
                                        const FuzzLimits& limits) {
    if (auto logicitem_id = fuzz_select_temporary_logicitem(stream, modifier)) {
        const auto data =
            to_layout_calculation_data(modifier.circuit_data().layout, logicitem_id);
        const auto delta = fuzz_select_move_delta(stream, data, limits);

        modifier.move_or_delete_temporary_logicitem(logicitem_id, delta);
    }
}

auto change_logicitem_insertion_mode(FuzzStream& stream, Modifier& modifier) {
    if (auto logicitem_id = fuzz_select_logicitem(stream, modifier)) {
        const auto new_mode = fuzz_select_insertion_mode(stream);
        modifier.change_logicitem_insertion_mode(logicitem_id, new_mode);
    }
}

auto add_logicitem(FuzzStream& stream, Modifier& modifier,
                   const FuzzLimits& limits) -> void {
    auto definition = [&]() {
        switch (fuzz_small_int(stream, 0, 3)) {
            case 0:
                return LogicItemDefinition {
                    .logicitem_type = LogicItemType::buffer_element,
                    .input_count = connection_count_t {1},
                    .output_count = connection_count_t {1},
                    .orientation = orientation_t::right,
                };
            case 1:
                return LogicItemDefinition {
                    .logicitem_type = LogicItemType::button,
                    .input_count = connection_count_t {0},
                    .output_count = connection_count_t {1},
                    .orientation = orientation_t::undirected,
                };
            case 2:
                return LogicItemDefinition {
                    .logicitem_type = LogicItemType::flipflop_jk,
                    .input_count = connection_count_t {5},
                    .output_count = connection_count_t {2},
                    .orientation = orientation_t::right,
                };
            case 3:
                return LogicItemDefinition {
                    .logicitem_type = LogicItemType::clock_generator,
                    .input_count = connection_count_t {3},
                    .output_count = connection_count_t {3},
                    .orientation = orientation_t::right,
                    .attrs_clock_generator = attributes_clock_generator_t {},
                };
        };
        std::terminate();
    }();

    const auto size = element_size(to_layout_calculation_data(definition, point_t {}));

    // TODO abstract this
    if ((int {size.x} > int {limits.box.p1.x} - int {limits.box.p0.x}) ||
        (int {size.y} > int {limits.box.p1.y} - int {limits.box.p0.y})) {
        return;
    }

    const auto position = point_t {
        fuzz_small_int(stream, int {limits.box.p0.x},
                       int {limits.box.p1.x} - int {size.x}),
        fuzz_small_int(stream, int {limits.box.p0.y},
                       int {limits.box.p1.y} - int {size.y}),
    };

    const auto mode = fuzz_select_insertion_mode(stream);
    modifier.add_logicitem(std::move(definition), position, mode);
}

auto logicitem_toggle_inverter(FuzzStream& stream, Modifier& modifier,
                               const FuzzLimits& limits) {
    const auto point = fuzz_select_point(stream, limits);
    modifier.toggle_inverter(point);
}

auto logicitem_set_attributes(FuzzStream& stream, Modifier& modifier) {
    if (const auto logicitem_id = fuzz_select_logicitem_type(
            stream, modifier, LogicItemType::clock_generator)) {
        auto attrs = attributes_clock_generator_t {
            .name = escape_as_hex(char8_t {stream.pop_or()}),
        };

        modifier.set_attributes(logicitem_id, std::move(attrs));
    }
}

//
// Decorations
//

auto delete_temporary_decoration(FuzzStream& stream, Modifier& modifier) {
    if (auto decoration_id = fuzz_select_temporary_decoration(stream, modifier)) {
        modifier.delete_temporary_decoration(decoration_id);
    }
}

auto move_or_delete_temporary_decoration(FuzzStream& stream, Modifier& modifier,
                                         const FuzzLimits& limits) {
    if (auto decoration_id = fuzz_select_temporary_decoration(stream, modifier)) {
        const auto data =
            to_decoration_layout_data(modifier.circuit_data().layout, decoration_id);
        const auto delta = fuzz_select_move_delta(stream, data, limits);

        modifier.move_or_delete_temporary_decoration(decoration_id, delta);
    }
}

auto change_decoration_insertion_mode(FuzzStream& stream, Modifier& modifier) {
    if (auto decoration_id = fuzz_select_decoration(stream, modifier)) {
        const auto new_mode = fuzz_select_insertion_mode(stream);
        modifier.change_decoration_insertion_mode(decoration_id, new_mode);
    }
}

auto add_decoration(FuzzStream& stream, Modifier& modifier,
                    const FuzzLimits& limits) -> void {
    auto definition = DecorationDefinition {
        .decoration_type = DecorationType::text_element,
        .size = size_2d_t {1, 0},
        .attrs_text_element = attributes_text_element_t {.text = "initial"},
    };

    const auto size = definition.size;
    const auto position = point_t {
        fuzz_small_int(stream, int {limits.box.p0.x},
                       int {limits.box.p1.x} - int {size.width}),
        fuzz_small_int(stream, int {limits.box.p0.y},
                       int {limits.box.p1.y} - int {size.height}),
    };

    const auto mode = fuzz_select_insertion_mode(stream);
    modifier.add_decoration(std::move(definition), position, mode);
}

auto decoration_set_attributes(FuzzStream& stream, Modifier& modifier) {
    if (const auto decoration_id = fuzz_select_decoration(stream, modifier)) {
        auto attrs = attributes_text_element_t {
            .text = escape_as_hex(char8_t {stream.pop_or()}),
        };

        modifier.set_attributes(decoration_id, std::move(attrs));
    }
}

//
// Selections
//

auto clear_visible_selection(Modifier& modifier) -> void {
    modifier.clear_visible_selection();
}

auto set_visible_selection(FuzzStream& stream, Modifier& modifier) -> void {
    auto selection = fuzz_select_selection(stream, modifier, 6);

    modifier.set_visible_selection(std::move(selection));
}

auto add_visible_selection_rect(FuzzStream& stream, Modifier& modifier,
                                const FuzzLimits& limits) -> void {
    const auto rect = fuzz_select_rect_fine(stream, limits);
    const auto function = fuzz_select_selection_function(stream);
    modifier.add_visible_selection_rect(function, rect);
}

auto try_pop_last_visible_selection_rect(Modifier& modifier) -> void {
    modifier.try_pop_last_visible_selection_rect();
}

auto try_update_last_visible_selection_rect(FuzzStream& stream, Modifier& modifier,
                                            const FuzzLimits& limits) -> void {
    const auto rect = fuzz_select_rect_fine(stream, limits);
    modifier.try_update_last_visible_selection_rect(rect);
}

auto apply_all_visible_selection_operations(Modifier& modifier) -> void {
    modifier.apply_all_visible_selection_operations();
}

auto editing_operation(FuzzStream& stream, Modifier& modifier,
                       const FuzzLimits& limits) -> void {
    switch (fuzz_small_int(stream, 0, 29)) {
        // wires
        case 0:
            add_wire_segment(stream, modifier, limits);
            return;
        case 1:
            delete_temporary_wire_segment(stream, modifier);
            return;
        case 2:
            change_wire_insertion_mode(stream, modifier);
            return;
        case 3:
            move_temporary_wire_unchecked(stream, modifier, limits);
            return;
        case 4:
            move_or_delete_temporary_wire(stream, modifier, limits);
            return;
        case 5:
            toggle_wire_crosspoint(stream, modifier, limits);
            return;

        // wire normalization
        case 6:
            set_temporary_endpoints(stream, modifier);
            return;
        case 7:
            merge_uninserted_segment(stream, modifier);
            return;
        case 8:
            split_uninserted_segment(stream, modifier);
            return;
        case 9:
            regularize_temporary_selection(stream, modifier, limits);
            return;
        case 10:
            split_temporary_segments(stream, modifier, limits);
            return;

        case 11:
            get_inserted_cross_points(stream, modifier);
            return;
        case 12:
            get_temporary_selection_splitpoints(stream, modifier);
            return;

        // logicitems
        case 13:
            delete_temporary_logicitem(stream, modifier);
            return;
        case 14:
            move_or_delete_temporary_logicitem(stream, modifier, limits);
            return;
        case 15:
            change_logicitem_insertion_mode(stream, modifier);
            return;
        case 16:
            add_logicitem(stream, modifier, limits);
            return;
        case 17:
            logicitem_toggle_inverter(stream, modifier, limits);
            return;
        case 18:
            logicitem_set_attributes(stream, modifier);
            return;

        // decorations
        case 19:
            delete_temporary_decoration(stream, modifier);
            return;
        case 20:
            move_or_delete_temporary_decoration(stream, modifier, limits);
            return;
        case 21:
            change_decoration_insertion_mode(stream, modifier);
            return;
        case 22:
            add_decoration(stream, modifier, limits);
            return;
        case 23:
            decoration_set_attributes(stream, modifier);
            return;

        // selection
        case 24:
            clear_visible_selection(modifier);
            return;
        case 25:
            set_visible_selection(stream, modifier);
            return;
        case 26:
            add_visible_selection_rect(stream, modifier, limits);
            return;
        case 27:
            try_pop_last_visible_selection_rect(modifier);
            return;
        case 28:
            try_update_last_visible_selection_rect(stream, modifier, limits);
            return;
        case 29:
            apply_all_visible_selection_operations(modifier);
            return;
    }
    std::terminate();
}

auto validate_undo(Modifier& modifier,
                   const std::vector<layout_key_state_t>& key_state_stack) {
    Expects(modifier.circuit_data().history.undo_stack.group_count() + std::size_t {1} ==
            std::size(key_state_stack));
    for (const auto& state : std::ranges::views::reverse(key_state_stack)  //
                                 | std::ranges::views::drop(1)) {
        Expects(has_undo(modifier));
        modifier.undo_group();
        Expects(is_valid(modifier));
        Expects(layout_key_state_t {modifier} == state);
    }
    Expects(modifier.circuit_data().history.undo_stack.group_count() == 0);
    Expects(!has_undo(modifier));
}

auto validate_redo(Modifier& modifier,
                   const std::vector<layout_key_state_t>& key_state_stack) {
    Expects(modifier.circuit_data().history.redo_stack.group_count() + std::size_t {1} ==
            std::size(key_state_stack));
    for (const auto& state : key_state_stack | std::ranges::views::drop(1)) {
        Expects(has_redo(modifier));
        modifier.redo_group();
        Expects(is_valid(modifier));
        Expects(layout_key_state_t {modifier} == state);
    }
    Expects(modifier.circuit_data().history.redo_stack.group_count() == 0);
    Expects(!has_redo(modifier));
}

auto validate_undo_redo(Modifier& modifier,
                        const std::vector<layout_key_state_t>& key_state_stack) {
    Expects(key_state_stack.empty() ||
            layout_key_state_t {modifier} == key_state_stack.back());
    Expects(!has_ungrouped_undo_entries(modifier));

    // Run twice, as redo may generate different stack entries
    validate_undo(modifier, key_state_stack);
    validate_redo(modifier, key_state_stack);
    validate_undo(modifier, key_state_stack);
    validate_redo(modifier, key_state_stack);
}

auto history_finish_undo_group(Modifier& modifier,
                               std::vector<layout_key_state_t>& key_state_stack) {
    if (has_ungrouped_undo_entries(modifier)) {
        modifier.finish_undo_group();
        key_state_stack.emplace_back(modifier);
    }
}

auto process_data(std::span<const uint8_t> data) -> void {
    auto stream = FuzzStream(data);

    const auto limits = [&]() {
        switch (fuzz_small_int(stream, 0, 1)) {
            case 0:
                // very small limits so most line interactions are found quickly
                return FuzzLimits {.box = rect_t {
                                       point_t {0, 0},
                                       point_t {2, 2},
                                   }};
            case 1:
                // fits clock generator for attr test
                return FuzzLimits {.box = rect_t {
                                       point_t {0, 0},
                                       point_t {5, 4},
                                   }};
        };
        std::terminate();
    }();

    auto modifier = Modifier {Layout {}, ModifierConfig {
                                             .enable_history = true,
                                             .validate_messages = true,
                                         }};
    auto key_state_stack = std::vector<layout_key_state_t> {
        layout_key_state_t {modifier},
    };

    while (!stream.empty()) {
        editing_operation(stream, modifier, limits);
        Expects(all_within_limits(modifier.circuit_data().layout, limits));
        Expects(is_valid(modifier));

        if (fuzz_bool(stream)) {
            history_finish_undo_group(modifier, key_state_stack);
        }
    }
    history_finish_undo_group(modifier, key_state_stack);

    // validate undo & redo
    validate_undo_redo(modifier, key_state_stack);
}

}  // namespace

}  // namespace editable_circuit

}  // namespace logicsim

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) -> int {
    using namespace logicsim::editable_circuit;
    const auto span = std::span<const uint8_t> {data, size};

    process_data(span);

    return 0;
}
