#include "render/text_alignment.h"

#include "algorithm/fmt_join.h"
#include "font_style_property.h"
#include "render/font.h"

namespace logicsim {

auto BaselineOffset::format() const -> std::string {
    return fmt::format(
        "BaselineOffset(baseline_center = {}, baseline_top = {}, "
        "baseline_bottom = {})",
        baseline_center, baseline_top, baseline_bottom);
}

[[nodiscard]] auto BaselineOffset::operator*(float font_size) const
    -> ScaledBaselineOffset {
    return ScaledBaselineOffset {
        .baseline_center = baseline_center * font_size,
        .baseline_top = baseline_top * font_size,
        .baseline_bottom = baseline_bottom * font_size,
    };
}

auto ScaledBaselineOffset::format() const -> std::string {
    return fmt::format(
        "ScaledBaselineOffset(baseline_center = {}, baseline_top = {}, "
        "baseline_bottom = {})",
        baseline_center, baseline_top, baseline_bottom);
}

auto calculate_horizontal_offset(const BLBox& bounding_box,
                                 HTextAlignment horizontal_alignment) -> double {
    const auto& box = bounding_box;

    switch (horizontal_alignment) {
        using enum HTextAlignment;

        case left:
            return box.x0;
        case right:
            return box.x1;
        case center:
            return (box.x0 + box.x1) / 2.0;
    }
    std::terminate();
}

auto calculate_vertical_offset(const BLBox& bounding_box,
                               const ScaledBaselineOffset& baseline_offsets,
                               VTextAlignment vertical_alignment) -> double {
    const auto& box = bounding_box;

    switch (vertical_alignment) {
        using enum VTextAlignment;

        case baseline:
            return 0;

        case center_baseline:
            return baseline_offsets.baseline_center;
        case top_baseline:
            return baseline_offsets.baseline_top;
        case bottom_baseline:
            return baseline_offsets.baseline_bottom;

        case center:
            return (box.y0 + box.y1) / 2.0;
        case top:
            return box.y0;
        case bottom:
            return box.y1;
    }
    std::terminate();
}

auto calculate_offset(const BLBox& bounding_box,
                      const ScaledBaselineOffset& baseline_offsets,
                      HTextAlignment horizontal_alignment,
                      VTextAlignment vertical_alignment) -> BLPoint {
    return BLPoint {
        calculate_horizontal_offset(bounding_box, horizontal_alignment),
        calculate_vertical_offset(bounding_box, baseline_offsets, vertical_alignment),
    };
}

auto calculate_baseline_offset(FontStyle style [[maybe_unused]], const FontFace& face)
    -> BaselineOffset {
    const auto text =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    const auto font_size = float {16};

    const auto font = HarfbuzzFont {face.hb_face()};
    const auto box = HarfbuzzShapedText {text, font, font_size}.bounding_box();

    using enum VTextAlignment;
    return BaselineOffset {
        .baseline_center = calculate_vertical_offset(box, {}, center) / font_size,
        .baseline_top = calculate_vertical_offset(box, {}, top) / font_size,
        .baseline_bottom = calculate_vertical_offset(box, {}, bottom) / font_size,
    };
}

//
// Collections
//

BaselineOffsets::BaselineOffsets(const FontFaces& faces) {
    for (const auto& style : all_font_styles) {
        set(style, calculate_baseline_offset(style, faces.get(style)));
    }
}

auto BaselineOffsets::format() const -> std::string {
    const auto to_string = [this](FontStyle style) {
        return fmt::format("{} = {}", style, this->get(style));
    };
    const auto joined = fmt_join(",\n  ", all_font_styles, "{}", to_string);

    return fmt::format("BaselineOffsets(\n  {})\n", joined);
}

auto BaselineOffsets::get(FontStyle style, float font_size) const
    -> ScaledBaselineOffset {
    return get(style) * font_size;
}

auto BaselineOffsets::get(FontStyle style) const -> const BaselineOffset& {
    return ::logicsim::get<const BaselineOffset&>(*this, style);
}

auto BaselineOffsets::set(FontStyle style, BaselineOffset offset) -> void {
    ::logicsim::set(*this, style, offset);
}

}  // namespace logicsim
