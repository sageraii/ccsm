# AGENTS.md — AI Contributor Guide

This file helps AI coding assistants (Claude Code, Copilot, Codex, etc.) work effectively in this codebase.

## Project

ccsm is a C++17 CLI+TUI tool that reads Claude Code's session files and provides tagging, search, and resume. ~1,900 lines of source, ~900 lines of tests, 10 test suites.

## Build & Test

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build build
cd build && ctest -V
```

All 10 suites must pass before committing.

## Architecture

```
history.jsonl ─┐
index.json ────┤→ SessionScanner → SessionStore → CLIApp / TUIApp
JSONL files ───┘                                      ↕
                                                  TagManager → ~/.claude/ccsm_tags.json
```

- **SessionScanner** reads 3 sources, merges by sessionId, sets active/expired status
- **SessionStore** holds sessions in memory, provides filter/sort/search/prefix-match
- **TagManager** manages user metadata (tags, notes, favorites) in a separate JSON file
- **CLIApp** uses CLI11; **TUIApp** uses FTXUI

## File Map

| To change... | Look at... |
|--------------|------------|
| Session fields | `src/core/session.hpp` (header-only) |
| How sessions are discovered | `src/core/session_scanner.cpp` |
| history.jsonl parsing | `src/core/history_parser.cpp` |
| sessions-index.json parsing | `src/core/index_parser.cpp` |
| JSONL transcript parsing | `src/core/jsonl_parser.cpp` |
| Search, filter, sort | `src/core/session_store.cpp` |
| Tags, notes, favorites | `src/core/tag_manager.cpp` |
| CLI commands | `src/cli/cli_app.cpp` |
| TUI interface | `src/tui/tui_app.cpp` |
| Text sanitization | `src/util/text_sanitizer.cpp` |
| Duration parsing (--since) | `src/util/duration_parser.cpp` |
| Entry point | `src/main.cpp` |

## Data Formats

**history.jsonl** (one line per user prompt):
```json
{"display":"prompt text","pastedContents":{},"timestamp":1769305522896,"project":"/path","sessionId":"uuid"}
```

**sessions-index.json** (per project, optional):
```json
{"version":1,"originalPath":"/path","entries":[{"sessionId":"uuid","summary":"...","messageCount":42,"gitBranch":"main","projectPath":"/path"}]}
```
Note: `fullPath` field exists but is always a dead link. Do not use it.

**JSONL transcripts** — first line may be `file-history-snapshot` (no sessionId). Scan forward up to 10 lines to find sessionId. Tool usage is at `assistant.message.content[*].type=="tool_use"`.

## Known Pitfalls

- `fullPath` in sessions-index.json is a dead link. Always ignore it.
- `firstPrompt` can be `"No prompt"` or contain XML like `<command-args>`. Use `sanitize_prompt()`.
- JSONL first line may lack sessionId (33% of files). Use scan-forward, not first-line-only.
- 82% of `user` events have array content (tool results), not string. Scan for string content.
- `summary` in sessions-index.json is optional. Do not assume it exists.
- `gitBranch` can be empty string `""`.

## Conventions

- All code in `namespace ccsm {}`
- `#pragma once` for headers
- `std::optional` for nullable fields
- Errors in CLI: `throw CLIError("message", exit_code)` — never `std::exit()`
- Claude Code files are read-only. User data goes to `~/.claude/ccsm_tags.json`
- `sort_by_date` sorts by `modified_at` (latest activity), not `created_at`
- Tags in `tags.json` use full UUID keys, not prefixes

## Adding a CLI Command

1. Add test case to `tests/test_cli_integration.cpp`
2. Add subcommand in `CLIApp::run()` in `src/cli/cli_app.cpp`
3. Use `resolve_session()` for session ID prefix matching
4. Use `throw CLIError(...)` for errors
5. Call `tags_.save()` after any tag/note/favorite mutation

## Adding a Filter

1. Add test to `tests/test_session_store.cpp`
2. Add method to `SessionStore` in `src/core/session_store.cpp`
3. Wire it in `CLIApp::run()` list subcommand callback

## Tests

Framework: Catch2. Tests are in `tests/test_*.cpp`, fixtures in `tests/fixtures/`.

```bash
ctest -R test_session_store -V   # run one suite
ctest -V                          # run all
```

Run all tests after any change. Do not commit with failing tests.
