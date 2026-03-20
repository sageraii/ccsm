#include "cli/cli_app.hpp"
#include "core/session_scanner.hpp"
#include "core/tag_manager.hpp"
#include "core/session_store.hpp"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    // If no arguments, show TUI placeholder
    if (argc == 1) {
        std::cout << "TUI mode: use `ccsm list` for now\n";
        return 0;
    }

    try {
        // Scan sessions
        auto config = ccsm::SessionScanner::default_config();
        ccsm::SessionScanner scanner(config);
        auto sessions = scanner.scan();

        // Apply tag data
        auto tags_path = config.claude_dir / "ccsm_tags.json";
        ccsm::TagManager tags(tags_path);

        for (auto& s : sessions) {
            auto td = tags.get(s.session_id);
            if (td.has_value()) {
                s.tags = td->tags;
                s.note = td->note.empty() ? std::nullopt : std::optional<std::string>(td->note);
                s.is_favorite = td->favorite;
            }
            tags.update_last_seen(s.session_id);
        }

        // Build store and run CLI
        ccsm::SessionStore store(std::move(sessions));
        ccsm::CLIApp cli(store, tags);
        return cli.run(argc, argv);

    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << "\n";
        return 1;
    }
}
