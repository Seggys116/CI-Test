# CI-Test

A public repository whose only purpose is to exercise CI. It holds fifteen small, real projects in different
languages and build systems. Every project implements the same utility library and the same command line tool, so a
failure on one runner is a statement about that runner, toolchain or build system and not about the code.

All workflows are manual (`workflow_dispatch`). Nothing runs on push or pull request.

## The shared library

Each project implements five utilities from scratch (no standard library shortcuts) and a `ciutil` CLI:

| utility | what it checks |
| --- | --- |
| `crc32` | table-driven CRC-32/IEEE, one-shot and incremental |
| `adler32` | Adler-32 including the NMAX block boundary, one-shot and incremental |
| `base64` | strict RFC 4648 encode and decode with rejection of non-canonical input |
| `roman` | numerals 1 to 3999, strict canonical parsing, exhaustive round trip |
| `semver` | SemVer 2.0.0 parse and precedence, full ordering chain |

CLI: `ciutil crc32|adler32|b64enc|b64dec|roman|unroman|semver-cmp|version`. Output must be byte-identical on every
OS and architecture (LF only, raw bytes for `b64dec`). `scripts/cli-smoke.sh` runs the same contract against every
project's built binary.

## Projects

| directory | language / tool | build system | tests |
| --- | --- | --- | --- |
| `projects/rust` | Rust | cargo | built-in test harness, doc tests, integration tests |
| `projects/python` | Python | uv, hatchling | pytest, hypothesis, ruff, mypy |
| `projects/npm` | TypeScript on Node | npm | vitest, prettier |
| `projects/pnpm` | TypeScript on Node | pnpm workspace | vitest, prettier |
| `projects/bun` | TypeScript on Bun | bun (`bun build --compile`) | bun test |
| `projects/deno` | TypeScript on Deno | deno (`deno compile`) | deno test |
| `projects/go` | Go | go toolchain | go test, fuzz seeds, race detector |
| `projects/zig` | Zig | zig build | zig test |
| `projects/swift` | Swift | SwiftPM | Swift Testing |
| `projects/java-gradle` | Java | Gradle (Kotlin DSL) | JUnit Jupiter |
| `projects/dotnet` | C# | dotnet SDK | xunit v3 |
| `projects/c-make` | C11 | GNU Make | in-repo harness, ASan and UBSan |
| `projects/c-cmake` | C11 | CMake, CTest | in-repo harness, install and `find_package` consumer |
| `projects/cpp-cmake` | C++20 | CMake, CTest | GoogleTest via FetchContent, install consumer |
| `projects/cpp-meson` | C++17 | Meson, Ninja | in-repo harness |

## Workflows

Run any of them from the Actions tab (or `gh workflow run <name>.yml`). Every workflow takes the same inputs:

- `runners`: `all`, `github-hosted` or `unified`
- `os`: `all`, `linux`, `macos` or `windows`
- `artifacts`: upload the built CLI from each job

| workflow | purpose |
| --- | --- |
| `all.yml` | runs every project workflow plus `runner-info` |
| `runner-info.yml` | prints OS, CPU, memory, disk and installed tool versions for each runner, and checks network reach |
| `<project>.yml` | build, lint, test and smoke test one project on the selected runners |
| `_runners.yml` | reusable: turns the inputs into the runner matrix |

### Runner sets

GitHub-hosted, all free standard runners for public repositories (labels checked against
`actions/runner-images` and the GitHub docs on 2026-11-25):

- Linux x64: `ubuntu-26.04`, `ubuntu-24.04`
- Linux arm64: `ubuntu-26.04-arm`, `ubuntu-24.04-arm`
- macOS arm64: `macos-26`, `macos-15`
- macOS Intel: `macos-26-intel`, `macos-15-intel`
- Windows x64: `windows-2025`, `windows-2022`
- Windows arm64: `windows-11-arm`

Unified: `unified-linux-4`, `unified-macos-4`, `unified-windows-4`. The architecture of these is not assumed;
workflows use `runner.arch` where it matters.

The jobs assume `bash` is available on every runner (Git for Windows provides it) and that the runner can download
toolchains through the setup actions.

## Running a project locally

Each project directory is self-contained. The exact commands are the steps in `.github/workflows/<project>.yml`.
