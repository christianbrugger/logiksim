#include "random.h"

#include "geometry.h"

#include <boost/random/seed_seq.hpp>

#include <random>

namespace logicsim {

auto get_random_number_generator() -> Rng {
    auto rd = std::random_device {};
    const auto seed_values = std::array {rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
    const auto seed_seq = boost::random::seed_seq(seed_values.begin(), seed_values.end());
    return Rng(seed_seq);
}

auto get_random_number_generator(uint32_t seed) -> Rng {
    return Rng {seed};
}

auto get_random_bool(Rng& rng) -> bool {
    return uint_distribution(0, 1)(rng) > 0;
}

auto get_random_grid(Rng& rng, grid_t::value_type min, grid_t::value_type max) -> grid_t {
    return uint_distribution(min, max)(rng);
}

auto get_random_line(Rng& rng, grid_t::value_type min, grid_t::value_type max)
    -> ordered_line_t {
    const auto get = [&]() { return get_random_grid(rng, min, max); };

    auto p0 = point_t {get(), get()};
    auto p1 = point_t {get(), get()};

    if (get_random_bool(rng)) {
        p0.x = p1.x;
    } else {
        p0.y = p1.y;
    }

    if (p0 == p1) {
        return get_random_line(rng, min, max);
    }

    return ordered_line_t {line_t {p0, p1}};
}

auto get_random_lines(Rng& rng, std::size_t count, grid_t::value_type min,
                      grid_t::value_type max) -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto _ [[maybe_unused]] : range(count)) {
        result.push_back(get_random_line(rng, min, max));
    }
    return result;
}

auto get_random_part(Rng& rng, part_t full_part) -> part_t {
    auto begin = offset_t::value_type {};
    auto end = offset_t::value_type {};
    const auto part_dist = uint_distribution(full_part.begin.value, full_part.end.value);
    while (begin >= end) {
        begin = part_dist(rng);
        end = part_dist(rng);
    }
    return part_t {offset_t {begin}, offset_t {end}};
}

auto get_random_part(Rng& rng, ordered_line_t line) -> part_t {
    const auto full_part = to_part(line);
    return get_random_part(rng, full_part);
}

auto get_random_insertion_mode(Rng& rng) -> InsertionMode {
    using enum InsertionMode;

    int index = uint_distribution(0, 2)(rng);

    switch (index) {
        case 0:
            return temporary;
        case 1:
            return collisions;
        case 2:
            return insert_or_discard;
    }
    throw_exception("unknown case");
}

}  // namespace logicsim
