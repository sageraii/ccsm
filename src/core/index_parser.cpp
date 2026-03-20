#include "core/index_parser.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace ccsm {

using json = nlohmann::json;

IndexFile parse_index(const std::filesystem::path& index_path) {
    IndexFile result;

    std::ifstream file(index_path);
    if (!file.is_open()) return result;

    try {
        auto j = json::parse(file);
        result.version = j.value("version", 0);

        if (j.contains("originalPath") && j["originalPath"].is_string()) {
            result.original_path = j["originalPath"].get<std::string>();
        }

        for (const auto& entry : j.value("entries", json::array())) {
            IndexEntry ie;
            ie.session_id = entry.value("sessionId", "");
            ie.full_path = entry.value("fullPath", "");
            ie.first_prompt = entry.value("firstPrompt", "");
            ie.message_count = entry.value("messageCount", 0);
            ie.created = entry.value("created", "");
            ie.modified = entry.value("modified", "");
            ie.git_branch = entry.value("gitBranch", "");
            ie.project_path = entry.value("projectPath", "");
            ie.is_sidechain = entry.value("isSidechain", false);

            if (entry.contains("summary") && entry["summary"].is_string()) {
                ie.summary = entry["summary"].get<std::string>();
            }

            if (!ie.session_id.empty()) {
                result.entries.push_back(std::move(ie));
            }
        }
    } catch (...) {
        // Return empty on parse failure
    }

    return result;
}

} // namespace ccsm
