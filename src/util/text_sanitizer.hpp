#pragma once

#include <string>
#include <string_view>

namespace ccsm {

std::string sanitize_prompt(std::string_view input);
bool is_sentinel(std::string_view input);
std::string strip_xml_tags(std::string_view input);
std::string extract_command_args(std::string_view input);

} // namespace ccsm
