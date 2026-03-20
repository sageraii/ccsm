#include "util/text_sanitizer.hpp"
#include <regex>
#include <algorithm>

namespace ccsm {

static constexpr std::size_t MAX_DISPLAY_LENGTH = 200;

std::string strip_xml_tags(std::string_view input) {
    static const std::regex tag_re("<[^>]+>");
    std::string s(input);
    return std::regex_replace(s, tag_re, "");
}

std::string extract_command_args(std::string_view input) {
    static const std::regex args_re(R"(<command-args>(.*?)</command-args>)");
    std::string s(input);
    std::smatch match;
    if (std::regex_search(s, match, args_re)) {
        return match[1].str();
    }
    return "";
}

bool is_sentinel(std::string_view input) {
    return input.empty() || input == "No prompt";
}

std::string sanitize_prompt(std::string_view input) {
    if (is_sentinel(input)) {
        return "";  // Return empty so display_summary() falls through to next level
    }

    std::string result;

    // Check for command markup
    if (input.find("<command-") != std::string_view::npos) {
        result = extract_command_args(input);
        if (result.empty()) {
            result = strip_xml_tags(input);
        }
    } else if (input.find('<') != std::string_view::npos) {
        result = strip_xml_tags(input);
    } else {
        result = std::string(input);
    }

    // Trim whitespace
    auto start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = result.find_last_not_of(" \t\n\r");
    result = result.substr(start, end - start + 1);

    // Truncate
    if (result.size() > MAX_DISPLAY_LENGTH) {
        result.resize(MAX_DISPLAY_LENGTH);
        result += "...";
    }

    return result;
}

} // namespace ccsm
