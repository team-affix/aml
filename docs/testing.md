# Testing

This repo uses **Google Test** and **Google Mock** for C++ tests (`googletest/` submodule at the repo root). The important split is between **unit** and **integration** tests: they answer different questions and follow different dependency rules.

The testing rules here mirror those of the `atlas` submodule (`atlas/docs/testing.md`), which is the authoritative reference. Reproduce the patterns there exactly when writing tests that exercise aml code built on top of atlas infrastructure.

## Layout

Each package that has tests follows the same directory pattern:

| Path | Purpose |
|------|---------|
| `<package>/test/unit/` | Unit tests — one real component per test file |
| `<package>/test/integration/` | Integration tests — several real components wired together |
| `<package>/test/main.cpp` | GTest entry point for that package's debug test binary |

For example, `core` uses `core/test/unit/`, `core/test/integration/`. The pattern is the same as `atlas/core/test/`.

Each package's debug build links its tests into a dedicated binary:

```
make core_debug        # → ./build/core_debug
make core_debug_fast   # → ./build/core_debug_fast
```

## Atlas dependency

`atlas/` is a git submodule. The aml makefile delegates library compilation to `atlas/makefile`:

- `atlas/build/libatlas_core_debug.a` is built automatically as a prerequisite of `core_debug`.
- Atlas headers are on the include path via `-Iatlas/core/hpp` and `-Iatlas`.
- aml code can directly `#include "infrastructure/bind_map.hpp"`, `#include "value_objects/expr.hpp"`, etc.

## Mocks (Google Mock)

All collaborators in tests must be **GMock mocks** — types that inherit the relevant `i_*` interface and use `MOCK_METHOD`. Do **not** add hand-written `fake_*`, `recording_*`, or `noop_*` test doubles.

See `atlas/docs/testing.md` for the full mock authoring rules, `NiceMock` vs `StrictMock` guidance, overloaded-method forwarding, and `unique_ptr` parameter syntax.

## Test fixtures (`TEST_F`)

Use a `struct … : public ::testing::Test` fixture per component. Factor shared resources into the fixture — do not reconstruct the same objects in every test body. GTest constructs a fresh fixture instance per `TEST_F`, so tests stay isolated.

## Unit tests

A **unit test** verifies the behavior of **one** class or function — the unit under test (SUT). Every collaborator is a mock; the SUT is the only real production type under test.

### Rules

1. Exactly one real implementation of production code in each test (the SUT).
2. Mock every dependency through its `i_*` interface with GMock.
3. Do not wire multiple real infrastructure components in a unit test. If the test needs two real types working together, it belongs in **integration**.
4. Prefer observable outcomes; use `EXPECT_CALL` for delegation contracts.
5. Call the concrete SUT on the fixture directly.

## Integration tests

An **integration test** checks behavior that only appears when **two or more real components** work together.

### Rules

1. Use real implementations for each type in the slice under test.
2. Still mock types outside the slice with GMock.
3. Assert end-state or cross-component invariants.
4. Do not duplicate unit coverage.

## Choosing unit vs integration

| Question | Unit | Integration |
|----------|------|-------------|
| Can collaborators be fully specified with `EXPECT_CALL`? | Yes | — |
| Does correctness depend on real WHNF, interning, trail undo, or bind chaining? | — | Yes |
| Is one class's logic the focus? | Yes | — |
| Is a pipeline or factory stack the focus? | — | Yes |

When in doubt: **unit test with mocks first**; add integration when mocks would need to reimplement another component to be meaningful.

## Naming and files

- Unit: `<package>/test/unit/<area>/<component>.cpp` — e.g. `MyComponentTest`.
- Integration: `<package>/test/integration/<component>.cpp` — e.g. `*IntegrationTest`.
- Mock types: `Mock` + interface name — e.g. `MockBindMap`, `MockTrail`.

## Building and running

```bash
# Build atlas core lib + aml test binary, then run tests
make core_debug
./build/core_debug

# Run a single test suite
./build/core_debug --gtest_filter='MyComponentTest.*'
```

## Additional rules

- Never change any non-test source files when the task is to create/modify/revise/remove tests.
- If bugs are discovered during testing, do **not** fix the bugs unless explicitly asked. Let the failing tests surface them.
