# Axiom Engine

An open-source C++23 / Vulkan 3D engine designed to be developed by LLM coding assistants within a 32K-token context window. Its goals are learned NPCs, a pacing AI director and learned physics trained from gameplay, plus path-traced photoreal rendering.

**Status:** Stage 0 (tooling and CI). Nothing renders yet. See [`docs/ROADMAP.md`](docs/ROADMAP.md) for the plan and [`docs/STATE.md`](docs/STATE.md) for where work stands.

## Build (Linux, Ubuntu 26.04)
Requirements: Clang 21 with libc++ 21, CMake ≥ 3.25, Ninja, Git, Vulkan headers and loader, Python 3 (`pip install -r scripts/requirements.txt`). The exact package set is in [`ci/packages.txt`](ci/packages.txt).

```sh
cmake --preset debug          # or asan-ubsan, release
cmake --build --preset debug
ctest --preset debug
python3 scripts/check_all.py  # include rules, line limit, repo maps, self-tests, formatting
```

## For coding agents
Read [`CLAUDE.md`](CLAUDE.md) first. It defines the session protocol, token budgets and Definition of Done.

## Documents
* [`docs/CONVENTIONS.md`](docs/CONVENTIONS.md): locked technical decisions
* [`docs/CONTEXT_RULES.md`](docs/CONTEXT_RULES.md): LLM context rules (loaded every session)
* [`docs/CONTEXT_TOOLING.md`](docs/CONTEXT_TOOLING.md): context tools, workflow, Doom-fork policy
* [`docs/ROADMAP.md`](docs/ROADMAP.md): stages, releases and acceptance tests
* [`docs/CREATIVE_DIRECTION.md`](docs/CREATIVE_DIRECTION.md): story, presentation and realism goals
* [`docs/adr/`](docs/adr/): architecture decision records

## License
MIT. See [LICENSE](LICENSE).
