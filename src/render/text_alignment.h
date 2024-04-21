#ifndef LOGICSIM_RENDER_TEXT_ALIGNMENT_H
#define LOGICSIM_RENDER_TEXT_ALIGNMENT_H

#include "format/struct.h"
#include "vocabulary/font_style.h"
#include "vocabulary/text_alignment.h"

#include <blend2d.h>

#include <concepts>
#include <string>

namespace logicsim {

struct FontFace;
struct FontFaces;

/**
 * @brief: scaled offsets for a specific font size
 */
struct ScaledBaselineOffset {
    double baseline_center {};
    double baseline_top {};
    double baseline_bottom {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const ScaledBaselineOffset &other) const
        -> bool = default;
};

static_assert(std::regular<ScaledBaselineOffset>);

/**
 * @brief: offsets for font-size of 1.0f
 */
struct BaselineOffset {
    double baseline_center {};
    double baseline_top {};
    double baseline_bottom {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const BaselineOffset &other) const
        -> bool = default;

    [[nodiscard]] auto operator*(float font_size) const -> ScaledBaselineOffset;
};

static_assert(std::regular<BaselineOffset>);

auto calculate_baseline_offset(FontStyle style [[maybe_unused]], const FontFace &face)
    -> BaselineOffset;

//
// Alignment Calculations
//

[[nodiscard]] auto calculate_horizontal_offset(const BLBox &bounding_box,
                                               HTextAlignment horizontal_alignment)
    -> double;

[[nodiscard]] auto calculate_vertical_offset(const BLBox &bounding_box,
                                             const ScaledBaselineOffset &baseline_offsets,
                                             VTextAlignment vertical_alignment) -> double;

[[nodiscard]] auto calculate_offset(const BLBox &bounding_box,
                                    const ScaledBaselineOffset &baseline_offsets,
                                    HTextAlignment horizontal_alignment,
                                    VTextAlignment vertical_alignment) -> BLPoint;

//
//
//

struct BaselineOffsets {
    BaselineOffset regular {};
    BaselineOffset italic {};
    BaselineOffset bold {};
    BaselineOffset monospace {};

    [[nodiscard]] explicit BaselineOffsets() = default;
    [[nodiscard]] explicit BaselineOffsets(const FontFaces &faces);

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const BaselineOffsets &other) const
        -> bool = default;

    [[nodiscard]] auto get(FontStyle style, float font_size) const
        -> ScaledBaselineOffset;
    [[nodiscard]] auto get(FontStyle style) const -> const BaselineOffset &;

    auto set(FontStyle style, BaselineOffset offset) -> void;
};

static_assert(std::regular<BaselineOffsets>);

}  // namespace logicsim

#endif
