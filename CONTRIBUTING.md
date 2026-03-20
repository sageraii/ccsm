# Contributing to ccsm

ccsm에 기여해주셔서 감사합니다!

**[English](CONTRIBUTING_EN.md)** | **한국어**

---

## Getting Started

### 빌드 환경 설정

```bash
# 1. 레포 클론
git clone https://github.com/sageraii/ccsm
cd ccsm

# 2. vcpkg 설치 (없는 경우)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

# 3. 빌드
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build build

# 4. 테스트 실행
cd build && ctest -V
```

---

## Development Workflow

### TDD (Test-Driven Development)

이 프로젝트는 TDD를 따릅니다:

1. **RED** — 실패하는 테스트를 먼저 작성
2. **GREEN** — 테스트를 통과시키는 최소 코드 작성
3. **REFACTOR** — 코드 정리 (테스트는 계속 통과)

```bash
# 테스트 작성 후 실행
cd build && ctest -R test_your_module -V

# 전체 테스트 실행
cd build && ctest -V
```

### 코드 스타일

- **C++17** 이상
- raw pointer 대신 smart pointer
- C 스타일 캐스팅/배열/문자열 지양
- `std::optional`, `std::string_view`, `std::filesystem` 활용
- RAII, const correctness
- `#pragma once` for include guards
- 모든 코드는 `namespace ccsm {}` 내에 작성

### 커밋 메시지

[Conventional Commits](https://www.conventionalcommits.org/) 형식:

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
├── core/           # 핵심 로직 (파서, 스캐너, 스토어, 태그)
├── cli/            # CLI11 기반 명령어 라우팅
├── tui/            # FTXUI 인터랙티브 브라우저
└── util/           # 유틸리티 (텍스트 정제, 기간 파싱)
tests/
├── test_*.cpp      # Catch2 테스트 파일
└── fixtures/       # 테스트 데이터 (JSON, JSONL)
```

### 핵심 컴포넌트

| 컴포넌트 | 파일 | 역할 |
|----------|------|------|
| Session | `core/session.hpp` | 데이터 모델 (header-only) |
| HistoryParser | `core/history_parser.*` | `history.jsonl` 파싱 |
| IndexParser | `core/index_parser.*` | `sessions-index.json` 파싱 |
| JSONLParser | `core/jsonl_parser.*` | JSONL 트랜스크립트 파싱 |
| SessionScanner | `core/session_scanner.*` | 3-stage 스캔 오케스트레이터 |
| SessionStore | `core/session_store.*` | 검색/필터/정렬 |
| TagManager | `core/tag_manager.*` | 사용자 메타데이터 CRUD |
| CLIApp | `cli/cli_app.*` | CLI 명령어 |
| TUIApp | `tui/tui_app.*` | TUI 인터페이스 |

---

## Adding a New Feature

1. **이슈 생성** — 기능 설명과 동기 작성
2. **브랜치 생성** — `feat/feature-name`
3. **테스트 먼저** — `tests/test_*.cpp`에 테스트 추가
4. **구현** — 테스트 통과시키기
5. **전체 테스트** — `ctest -V`로 회귀 확인
6. **PR 제출**

### 테스트 가이드

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

- 테스트 이름은 동작을 설명 (e.g., "parse_history handles missing file gracefully")
- `SECTION`으로 관련 케이스 그룹화
- fixture 파일은 `tests/fixtures/`에 배치
- mock 최소화, 실제 코드 테스트

---

## Reporting Issues

- **버그**: 재현 단계, 기대 결과, 실제 결과 포함
- **기능 제안**: 사용 시나리오와 동기 설명
- **질문**: Discussions 탭 활용

---

## AI Contributor

AI 코딩 어시스턴트(Claude Code, Copilot, Codex 등)로 이 프로젝트에 기여하는 경우, [AGENTS.md](AGENTS.md)를 먼저 읽는다. 프로젝트 구조, 파일 맵, 데이터 형식, 알려진 주의사항, CLI 명령어/필터 추가 방법이 정리되어 있다. AI 도구가 이 파일을 참조하면 정확한 코드를 생성할 수 있다.

---

## License

기여하신 코드는 [MIT License](LICENSE)로 배포됩니다.
