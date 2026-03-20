#pragma once

#include <optional>
#include <string_view>
#include <chrono>
#include <string>

namespace ccsm {

using Duration = std::chrono::seconds;

std::optional<Duration> parse_duration(std::string_view input);
std::string supported_formats_hint();

} // namespace ccsm
