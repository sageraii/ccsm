# ccsm — Claude Code Session Manager

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg?logo=cmake)](https://cmake.org)
[![vcpkg](https://img.shields.io/badge/vcpkg-manifest-8A2BE2.svg)](https://vcpkg.io)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)]()
[![Tests](https://img.shields.io/badge/Tests-176%20assertions-brightgreen.svg)]()
[![GitHub stars](https://img.shields.io/github/stars/sageraii/ccsm?style=social)](https://github.com/sageraii/ccsm)

Claude Code 세션을 조회하고, 태그를 붙이고, 재개하는 CLI + TUI 도구.

**[English](README_EN.md)** | **한국어**

---

## 문제

Claude Code는 세션 기록을 `~/.claude/history.jsonl`에 기록하지만, 여러 프로젝트에 걸친 세션을 탐색하거나 레이블을 붙이는 기능이 없다. 수십 개의 세션이 쌓이면 원하는 세션을 찾기 어렵고, 중요한 작업을 다시 이어가기 위해 긴 UUID를 복사해야 한다. ccsm은 Claude Code 자체 파일을 읽어 이 문제를 해결한다 — 별도의 데이터베이스 없이.

---

## 기능

- **TUI 모드** — 인자 없이 실행하면 FTXUI 기반 전체 화면 인터페이스 실행
- **세션 목록** — 활성/만료 세션 조회, 프로젝트·브랜치·태그·기간 필터
- **출력 형식** — 테이블, JSON, CSV 지원
- **세션 상세** — 도구 사용 횟수, 편집된 파일, 서브에이전트 정보
- **검색** — 세션 ID, 프로젝트 경로, 요약, 태그, 브랜치, 노트 대상 대소문자 무관 검색
- **태그 / 노트 / 즐겨찾기** — `~/.claude/ccsm_tags.json`에 사용자 메타데이터 저장
- **세션 재개** — 원래 프로젝트 디렉토리에서 `claude --resume` 실행
- **prefix 매칭** — git 스타일, 최소 3자 (예: `abc123` → 전체 UUID 매핑)
- **정리** — 만료 세션의 태그 데이터 정리

---

## 빠른 시작

**의존성:** CMake 3.20+, C++17 컴파일러, [vcpkg](https://github.com/microsoft/vcpkg)

```bash
git clone https://github.com/sageraii/ccsm
cd ccsm

# vcpkg로 의존성 설치
vcpkg install

# 빌드
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build

# 실행
./build/ccsm
```

---

## 사용법

### TUI 모드

```bash
ccsm
```

인자 없이 실행하면 전체 화면 TUI가 열린다.

```
┌─ ccsm - Claude Code Session Manager ────────────────────┐
│ 검색: [________] | 필터: Active | 정렬: 최신순           │
│──────────────────────────────────────────────────────────│
│ * aaaa1111  03-18 14:22  GR00T-Dreams  main  #training  │
│   "IDM training pipeline setup"  42msg  활성             │
│                                                          │
│   bbbb2222  03-17 09:11  ucp-tutorial  dev               │
│   "UCP checkout implementation"  28msg  활성             │
│                                                          │
│ [Enter] Resume  [t] Tag  [n] Note  [f] Fav  [q] Quit   │
│ [/] Search  [e] Toggle Expired  [i] Info                │
└──────────────────────────────────────────────────────────┘
```

**키 바인딩:**

| 키 | 동작 |
|----|------|
| `j` / `↓` | 아래로 이동 |
| `k` / `↑` | 위로 이동 |
| `PgDn` / `PgUp` | 10개씩 이동 |
| `Home` / `End` | 처음 / 끝 |
| `Enter` | 선택한 세션 재개 |
| `t` | 태그 입력 프롬프트 |
| `n` | 노트 입력 프롬프트 |
| `f` | 즐겨찾기 토글 |
| `i` | 상세 정보 오버레이 |
| `e` | 만료 세션 표시 토글 |
| `/` | 검색 포커스 |
| `q` / `Esc` | 종료 |

### CLI 모드

```bash
ccsm list                          # 활성 세션 최신 20개
ccsm list --all                    # 개수 제한 없이 전체
ccsm list --expired                # 만료 세션 포함
ccsm list --project myproj         # 프로젝트 이름 필터
ccsm list --branch main            # 브랜치 필터
ccsm list --since 7d               # 최근 7일 (2d, 1w, 1m 형식 지원)
ccsm list --tag important          # 태그 필터
ccsm list --sort messages          # 메시지 수로 정렬 (date|messages|project)
ccsm list --format json            # JSON 출력
ccsm list --format csv             # CSV 출력
ccsm list --limit 50               # 표시 개수 지정

ccsm info abc123                   # 세션 상세 (도구 사용, 편집 파일, 서브에이전트)
ccsm search "pipeline"             # 메타데이터 검색

ccsm tag abc123 important          # 태그 추가
ccsm untag abc123 important        # 태그 제거
ccsm note abc123 "주요 작업"       # 노트 설정
ccsm favorite abc123               # 즐겨찾기 토글
ccsm favorites                     # 즐겨찾기 목록

ccsm resume abc123                 # 원래 디렉토리에서 세션 재개
ccsm cleanup                       # 만료 세션 태그 데이터 정리

ccsm --help                        # 도움말
ccsm --version                     # 버전 정보
```

세션 ID는 prefix로 지정한다. 최소 3자이며, 고유하게 특정되면 전체 UUID로 매핑된다. 여러 세션이 일치하면 목록을 표시한다.

---

## 동작 원리

세션 스캔은 3단계로 진행된다.

**Stage 1 — history.jsonl (주 소스)**
`~/.claude/history.jsonl`에서 모든 세션 엔트리를 읽는다. 세션 95% 이상을 여기서 발견한다.

**Stage 2 — sessions-index.json (보조 소스)**
`~/.claude/projects/<encoded-path>/sessions-index.json`에서 요약, 메시지 수, git 브랜치 정보를 병합한다.

**Stage 3 — 파일시스템 스캔 (활성 여부 판별)**
`~/.claude/projects/` 아래의 UUID 이름 JSONL 파일과 세션 디렉토리를 탐색한다. JSONL 파일 또는 세션 디렉토리가 있으면 `active`, 없으면 `expired`로 표시한다. 서브에이전트 디렉토리(`subagents/`)가 있으면 에이전트 수와 유형도 수집한다.

**사용자 메타데이터**
태그, 노트, 즐겨찾기는 `~/.claude/ccsm_tags.json`에 별도 저장한다. Claude Code 자체 파일은 수정하지 않는다.

---

## 요구사항

| 항목 | 버전 |
|------|------|
| C++ 표준 | C++17 |
| CMake | 3.20 이상 |
| vcpkg | 최신 권장 |
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | vcpkg로 설치 |
| [CLI11](https://github.com/CLIUtils/CLI11) | vcpkg로 설치 |
| [nlohmann/json](https://github.com/nlohmann/json) | vcpkg로 설치 |
| [Catch2](https://github.com/catchorg/Catch2) | vcpkg로 설치 (테스트용) |

---

## 유사 도구

| 도구 | 언어 | 특징 |
|------|------|------|
| [cc-sessions](https://github.com/) | Rust | 세션 목록 및 열기 |
| [ccs](https://github.com/) | Go | 세션 검색 |
| **ccsm** | C++ | 태그·노트·즐겨찾기, TUI, 서브에이전트 정보 |

---

## 라이선스

MIT

---

## 기여

1. 이슈 또는 PR을 [GitHub](https://github.com/sageraii/ccsm)에 제출한다.
2. 새 기능은 `Catch2` 테스트를 함께 작성한다.
3. 코드는 C++17 표준을 따르며 `clang-format`으로 포맷한다.
