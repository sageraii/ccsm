#include "cli/cli_app.hpp"
#include "tui/tui_app.hpp"
#include "core/session_scanner.hpp"
#include "core/tag_manager.hpp"
#include "core/session_store.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <unistd.h>

static const char* SHELL_INIT_BASH = R"CCSM(
# ccsm shell integration — add to ~/.bashrc:
#   eval "$(ccsm init bash)"
ccsm() {
    local _ccsm_hook
    _ccsm_hook=$(mktemp "/tmp/ccsm-hook.XXXXXX")
    command ccsm --_shell-hook "$_ccsm_hook" "$@"
    local _ccsm_rc=$?
    if [ -s "$_ccsm_hook" ]; then
        source "$_ccsm_hook"
    fi
    rm -f "$_ccsm_hook"
    return $_ccsm_rc
}
)CCSM";

static void write_resume_hook(const std::string& hook_path,
                               const std::string& session_id,
                               const std::string& project_path) {
    std::ofstream f(hook_path);
    if (!f.is_open()) return;

    namespace fs = std::filesystem;
    if (!project_path.empty() && fs::is_directory(project_path)) {
        f << "cd \"" << project_path << "\"\n";
    }
    // readline pre-fill: user sees the command, just presses Enter
    f << "read -e -i \"claude --resume " << session_id
      << "\" -p \"$ \" _ccsm_cmd && eval \"$_ccsm_cmd\"\n";
}

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    // Handle: ccsm init bash
    if (argc >= 3 && std::strcmp(argv[1], "init") == 0 && std::strcmp(argv[2], "bash") == 0) {
        std::cout << SHELL_INIT_BASH;
        return 0;
    }

    // Handle: ccsm init zsh (same wrapper works for zsh)
    if (argc >= 3 && std::strcmp(argv[1], "init") == 0 && std::strcmp(argv[2], "zsh") == 0) {
        std::cout << SHELL_INIT_BASH; // compatible with zsh
        return 0;
    }

    // Extract --_shell-hook <path> if present
    std::string shell_hook_path;
    std::vector<char*> filtered_argv;
    for (int i = 0; i < argc; ++i) {
        if (std::strcmp(argv[i], "--_shell-hook") == 0 && i + 1 < argc) {
            shell_hook_path = argv[++i];
        } else {
            filtered_argv.push_back(argv[i]);
        }
    }
    int filtered_argc = static_cast<int>(filtered_argv.size());

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

        // Build store
        ccsm::SessionStore store(std::move(sessions));

        // If no arguments (after filtering), launch TUI
        if (filtered_argc == 1) {
            ccsm::TUIApp tui(store, tags);
            int rc = tui.run();

            auto req = tui.resume_request();
            if (req.has_value()) {
                if (!shell_hook_path.empty()) {
                    // Shell wrapper mode: write hook script for parent shell to source
                    write_resume_hook(shell_hook_path, req->session_id, req->project_path);
                } else {
                    // Direct mode: exec into interactive bash with user's profile
                    // + readline pre-fill via DSR escape sequence trick
                    if (!req->project_path.empty() && fs::is_directory(req->project_path)) {
                        chdir(req->project_path.c_str());
                    }

                    // Write temp init file: source user's bashrc + pre-fill command
                    auto init_path = fs::temp_directory_path() / "ccsm-init.sh";
                    {
                        std::ofstream f(init_path);
                        f << "[ -f ~/.bashrc ] && source ~/.bashrc\n"
                          << "bind '\"\\e[0n\":\"claude --resume " << req->session_id << "\"'\n"
                          << "printf '\\e[5n'\n"
                          << "rm -f " << init_path.string() << "\n";
                    }

                    execlp("bash", "bash", "--init-file", init_path.c_str(), nullptr);
                    std::cerr << "bash 실행 실패\n";
                    return 1;
                }
            }

            return rc;
        }

        // Otherwise run CLI
        ccsm::CLIApp cli(store, tags);
        return cli.run(filtered_argc, filtered_argv.data());

    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << "\n";
        return 1;
    }
}
