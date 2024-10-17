#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_LOGIKITEM_BASE_GENERIC_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_LOGIKITEM_BASE_GENERIC_H

#include "core/format/struct.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/element_draw_state.h"
#include "core/vocabulary/font_style.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/text_alignment.h"

#include <concepts>
#include <optional>
#include <string>
#include <type_traits>

namespace logicsim {

struct point_fine_t;
struct rect_fine_t;

struct logicitem_id_t;
class Layout;

struct Context;

[[nodiscard]] auto get_logicitem_fill_color(ElementDrawState state) -> color_t;
[[nodiscard]] auto get_logicitem_stroke_color(ElementDrawState state) -> color_t;
[[nodiscard]] auto get_logicitem_label_color(ElementDrawState state) -> color_t;

[[nodiscard]] auto get_logicitem_center(const Layout& layout,
                                        logicitem_id_t logicitem_id) -> point_fine_t;

struct LogicItemRectAttributes {
    std::optional<color_t> custom_fill_color {};
    std::optional<color_t> custom_stroke_color {};

    [[nodiscard]] auto operator==(const LogicItemRectAttributes&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<LogicItemRectAttributes>);
static_assert(std::regular<LogicItemRectAttributes>);

auto draw_logicitem_rect(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                         ElementDrawState state,
                         LogicItemRectAttributes attributes = {}) -> void;

auto draw_logicitem_rect(Context& ctx, rect_fine_t rect, ElementDrawState state,
                         LogicItemRectAttributes attributes = {}) -> void;

struct LogicItemTextAttributes {
    std::optional<grid_fine_t> custom_font_size {};
    std::optional<color_t> custom_text_color {};
    HTextAlignment horizontal_alignment {HTextAlignment::center};
    VTextAlignment vertical_alignment {VTextAlignment::center};
    FontStyle style {FontStyle::regular};

    [[nodiscard]] auto operator==(const LogicItemTextAttributes&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<LogicItemTextAttributes>);
static_assert(std::regular<LogicItemTextAttributes>);

auto draw_logicitem_label(Context& ctx, const Layout& layout, logicitem_id_t logicitem_id,
                          std::string_view text, ElementDrawState state,
                          LogicItemTextAttributes attributes = {}) -> void;

auto draw_logicitem_label(Context& ctx, point_fine_t center, std::string_view text,
                          ElementDrawState state,
                          LogicItemTextAttributes attributes = {}) -> void;

auto draw_binary_value(Context& ctx, point_fine_t point, bool is_enabled,
                       ElementDrawState state) -> void;
auto draw_binary_true(Context& ctx, point_fine_t point, ElementDrawState state) -> void;
auto draw_binary_false(Context& ctx, point_fine_t point, ElementDrawState state) -> void;

}  // namespace logicsim

#endif
