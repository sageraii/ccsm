#pragma once

#include "core/session.hpp"
#include <vector>
#include <filesystem>

namespace ccsm {

std::vector<Session> parse_history(const std::filesystem::path& history_path);

// Safe HOME directory getter
std::filesystem::path get_home_dir();

} // namespace ccsm
