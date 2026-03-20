# ccsm — Claude Code Session Manager

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg?logo=cmake)](https://cmake.org)
[![vcpkg](https://img.shields.io/badge/vcpkg-manifest-8A2BE2.svg)](https://vcpkg.io)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)]()
[![Tests](https://img.shields.io/badge/Tests-176%20assertions-brightgreen.svg)]()
[![GitHub stars](https://img.shields.io/github/stars/sageraii/ccsm?style=social)](https://github.com/sageraii/ccsm)

A CLI + TUI tool to browse, tag, and resume Claude Code sessions.

---

## Problem

Claude Code logs every session to `~/.claude/history.jsonl` but provides no way to browse sessions across projects, attach labels, or quickly resume a specific one. As sessions accumulate, finding the right one requires scanning long UUIDs with no context. ccsm reads Claude Code's own files directly — no separate database required.

---

## Features

- **TUI mode** — full-screen FTXUI interface when run with no arguments
- **Session listing** — active and expired sessions with filters by project, branch, tag, and time range
- **Output formats** — table, JSON, CSV
- **Session detail** — tool usage counts, edited files, subagent metadata
- **Search** — case-insensitive search across session ID, project path, summary, tags, branch, and notes
- **Tags / notes / favorites** — user metadata stored in `~/.claude/ccsm_tags.json`
- **Resume** — runs `claude --resume` in the original project directory
- **Prefix matching** — git-style, minimum 3 characters (e.g. `abc123` resolves to the full UUID)
- **Cleanup** — removes tag data for expired sessions

---

## Quick Start

**Dependencies:** CMake 3.20+, a C++17 compiler, [vcpkg](https://github.com/microsoft/vcpkg)

```bash
git clone https://github.com/sageraii/ccsm
cd ccsm

# Install dependencies via vcpkg
vcpkg install

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build

# Run
./build/ccsm
```

---

## Usage

### TUI mode

```bash
ccsm
```

Run with no arguments to open the full-screen TUI.

```
┌─ ccsm - Claude Code Session Manager ────────────────────┐
│ Search: [________] | Filter: Active | Sort: Latest       │
│──────────────────────────────────────────────────────────│
│ * aaaa1111  03-18 14:22  GR00T-Dreams  main  #training  │
│   "IDM training pipeline setup"  42msg  active           │
│                                                          │
│   bbbb2222  03-17 09:11  ucp-tutorial  dev               │
│   "UCP checkout implementation"  28msg  active           │
│                                                          │
│ [Enter] Resume  [t] Tag  [n] Note  [f] Fav  [q] Quit   │
│ [/] Search  [e] Toggle Expired  [i] Info                │
└──────────────────────────────────────────────────────────┘
```

**Key bindings:**

| Key | Action |
|-----|--------|
| `j` / `↓` | Move down |
| `k` / `↑` | Move up |
| `PgDn` / `PgUp` | Jump 10 entries |
| `Home` / `End` | First / last entry |
| `Enter` | Resume selected session |
| `t` | Tag input prompt |
| `n` | Note input prompt |
| `f` | Toggle favorite |
| `i` | Detail info overlay |
| `e` | Toggle expired sessions |
| `/` | Focus search box |
| `q` / `Esc` | Quit |

### CLI mode

```bash
ccsm list                          # Active sessions, latest 20
ccsm list --all                    # No count limit
ccsm list --expired                # Include expired sessions
ccsm list --project myproj         # Filter by project name
ccsm list --branch main            # Filter by branch
ccsm list --since 7d               # Last 7 days (2d, 1w, 1m formats supported)
ccsm list --tag important          # Filter by tag
ccsm list --sort messages          # Sort by message count (date|messages|project)
ccsm list --format json            # JSON output
ccsm list --format csv             # CSV output
ccsm list --limit 50               # Custom result count

ccsm info abc123                   # Session detail: tool usage, edited files, subagents
ccsm search "pipeline"             # Search across session metadata

ccsm tag abc123 important          # Add tag
ccsm untag abc123 important        # Remove tag
ccsm note abc123 "key work"        # Set note
ccsm favorite abc123               # Toggle favorite
ccsm favorites                     # List favorites

ccsm resume abc123                 # Resume session in its original directory
ccsm cleanup                       # Remove tag data for expired sessions

ccsm --help                        # Help
ccsm --version                     # Version
```

Session IDs are specified as prefixes. The minimum length is 3 characters. If the prefix uniquely identifies one session it resolves to the full UUID; if multiple sessions match, all candidates are listed.

---

## How It Works

Session discovery runs in three stages.

**Stage 1 — history.jsonl (primary source)**
Reads all session entries from `~/.claude/history.jsonl`. This covers 95%+ of sessions.

**Stage 2 — sessions-index.json (secondary source)**
Merges summary text, message counts, and git branch data from `~/.claude/projects/<encoded-path>/sessions-index.json` into the existing session records.

**Stage 3 — filesystem scan (active/expired determination)**
Walks `~/.claude/projects/` looking for UUID-named `.jsonl` files and session directories. A session is marked `active` if its JSONL file or session directory exists; otherwise it is `expired`. Session directories with a `subagents/` subdirectory also yield subagent counts and types.

**User metadata**
Tags, notes, and favorites are written to `~/.claude/ccsm_tags.json`. Claude Code's own files are never modified.

---

## Requirements

| Item | Version |
|------|---------|
| C++ standard | C++17 |
| CMake | 3.20 or later |
| vcpkg | latest recommended |
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | installed via vcpkg |
| [CLI11](https://github.com/CLIUtils/CLI11) | installed via vcpkg |
| [nlohmann/json](https://github.com/nlohmann/json) | installed via vcpkg |
| [Catch2](https://github.com/catchorg/Catch2) | installed via vcpkg (tests) |

---

## Related Tools

| Tool | Language | Notes |
|------|----------|-------|
| [cc-sessions](https://github.com/) | Rust | Session listing and opening |
| [ccs](https://github.com/) | Go | Session search |
| **ccsm** | C++ | Tags, notes, favorites, TUI, subagent metadata |

---

## License

MIT

---

## Contributing

1. Open an issue or pull request on [GitHub](https://github.com/sageraii/ccsm).
2. New features should include `Catch2` tests.
3. Code follows C++17 and is formatted with `clang-format`.
