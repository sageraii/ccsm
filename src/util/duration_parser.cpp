#include "util/duration_parser.hpp"
#include <charconv>

namespace ccsm {

std::optional<Duration> parse_duration(std::string_view input) {
    if (input.size() < 2) return std::nullopt;

    auto unit = input.back();
    auto num_str = input.substr(0, input.size() - 1);

    int value = 0;
    auto [ptr, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), value);
    if (ec != std::errc{} || ptr != num_str.data() + num_str.size() || value <= 0) {
        return std::nullopt;
    }

    switch (unit) {
        case 'd': return Duration(value * 24 * 3600);
        case 'w': return Duration(value * 7 * 24 * 3600);
        case 'm': return Duration(value * 30 * 24 * 3600); // m = month (30 days)
        default: return std::nullopt;
    }
}

std::string supported_formats_hint() {
    return "Supported formats: Nd (days), Nw (weeks), Nm (months, 30 days)";
}

} // namespace ccsm
