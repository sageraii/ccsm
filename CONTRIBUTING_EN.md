# Contributing to ccsm

Thank you for contributing to ccsm!

**English** | **[한국어](CONTRIBUTING.md)**

---

## Getting Started

### Build Environment Setup

```bash
# 1. Clone the repo
git clone https://github.com/sageraii/ccsm
cd ccsm

# 2. Install vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

# 3. Build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build build

# 4. Run tests
cd build && ctest -V
```

---

## Development Workflow

### TDD (Test-Driven Development)

This project follows TDD:

1. **RED** — Write a failing test first
2. **GREEN** — Write the minimum code to make it pass
3. **REFACTOR** — Clean up while keeping tests green

```bash
# Run a specific test suite
cd build && ctest -R test_your_module -V

# Run all tests
cd build && ctest -V
```

### Code Style

- **C++17** or later
- Smart pointers instead of raw pointers
- No C-style casts, arrays, or strings
- Use `std::optional`, `std::string_view`, `std::filesystem`
- RAII, const correctness
- `#pragma once` for include guards
- All code inside `namespace ccsm {}`

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add session export command
fix: handle empty gitBranch in index parser
test: add prefix matching edge case tests
docs: update CLI usage examples
refactor: extract UUID validation to utility
```

---

## Project Structure

```
src/
├── core/           # Core logic (parsers, scanner, store, tags)
├── cli/            # CLI11-based command routing
├── tui/            # FTXUI interactive browser
└── util/           # Utilities (text sanitization, duration parsing)
tests/
├── test_*.cpp      # Catch2 test files
└── fixtures/       # Test data (JSON, JSONL)
```

### Core Components

| Component | Files | Role |
|-----------|-------|------|
| Session | `core/session.hpp` | Data model (header-only) |
| HistoryParser | `core/history_parser.*` | Parses `history.jsonl` |
| IndexParser | `core/index_parser.*` | Parses `sessions-index.json` |
| JSONLParser | `core/jsonl_parser.*` | Parses JSONL transcripts |
| SessionScanner | `core/session_scanner.*` | 3-stage scan orchestrator |
| SessionStore | `core/session_store.*` | Search / filter / sort |
| TagManager | `core/tag_manager.*` | User metadata CRUD |
| CLIApp | `cli/cli_app.*` | CLI commands |
| TUIApp | `tui/tui_app.*` | TUI interface |

---

## Adding a New Feature

1. **Open an issue** — Describe the feature and motivation
2. **Create a branch** — `feat/feature-name`
3. **Write tests first** — Add tests to `tests/test_*.cpp`
4. **Implement** — Make the tests pass
5. **Run all tests** — Verify no regressions with `ctest -V`
6. **Submit a PR**

### Testing Guide

```cpp
#include <catch2/catch_test_macros.hpp>
#include "core/your_module.hpp"

TEST_CASE("descriptive name of behavior", "[module_tag]") {
    // Arrange
    auto input = setup();

    // Act
    auto result = function(input);

    // Assert
    REQUIRE(result == expected);
}
```

- Test names should describe behavior (e.g., "parse_history handles missing file gracefully")
- Use `SECTION` to group related cases
- Place fixture files in `tests/fixtures/`
- Minimize mocks, test real code

---

## Reporting Issues

- **Bugs**: Include reproduction steps, expected result, and actual result
- **Feature requests**: Describe the use case and motivation
- **Questions**: Use the Discussions tab

---

## AI Contributors

If you're using an AI coding assistant (Claude Code, Copilot, Codex, etc.) to contribute, read [AGENTS.md](AGENTS.md) first. It contains the project architecture, file map, data formats, known pitfalls, and step-by-step guides for adding commands and filters. AI tools that reference this file will produce more accurate code.

---

## License

All contributions are released under the [MIT License](LICENSE).
