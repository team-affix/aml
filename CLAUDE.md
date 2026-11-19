# AML — project memory for Claude Code

## What this is

AML (abstractive machine learning) learns models of noiseless, functional data that fit every data point exactly
and are minimal in symbolic description length. Fit comes first; among models that fit, cheaper is better.

- **Data:** each data point is an input tuple (bound to variables x0..xn-1) and a label. Values are interned
  constructor terms, `value{tag, args}`, where `tag` is the constructor's position in its type. A value carries no
  type: columns do (`typed_column`, for the inputs and the labels), so comparisons that must respect types compare
  typed columns.
- **User types are data, never C++ types.** A user type is a `type_def`: its constructors (`ctor_def`), each with its
  argument types. `schema` is built once from all of them; a type's id is its position, so types can be recursive.
  Core knows no particular user type (no bool, no nat); anything specific to one lives with the user (the tests).
- **Primitives and constants:** a `primitive` is a `std::function` from argument values to a result value, made
  through the solver's value pool; a `typed_primitive` pairs it with its `primitive_type` (argument and result
  types). The constants synthesis may use as arguments are `typed_constant`s the user offers. Solvers receive both at
  construction; nothing is enumerated. They are user code: the ones the tests use live in `core/test/support/`.
- **Features:** a variable, a primitive application, or a constant. A feature owns its arguments, so it is a tree
  (not interned). A feature is heterogeneous over a bin when its values there are not all equal.
- **Model:** a tree that owns its nodes (not interned); each episode builds one, and `best_model_tracker` keeps a copy
  of the cheapest. Three node types, nested in `model_node` so `match` needs no forward declaration: `match` (an
  exhaustive one-layer case on a feature; each branch sees its constructor's arguments bound as the next variables),
  `leaf` (outputs a feature evaluated in scope: a constant label, a variable, or a synthesized feature), `empty` (no
  data; infers nothing). Variable and synthesized outputs generalize past the training data, so a model can be right on
  inputs no row held.
- **Learning (`model_builder`):** an empty bin becomes `empty`. `leaf_detector` makes a leaf of a variable whose
  column is the label column (cost 1, preferred) or of labels that all agree (a constant). Otherwise a synthesis
  session runs; a synthesized feature equal to the labels becomes the leaf's output (`output_finder`, cheapest
  first), and failing that the bin is matched on a chosen session feature, copied out of the session. A **funnel** is
  a match whose rows all land in one branch (every other branch is `empty`); it only binds that constructor's
  arguments. A funnel's branch decomposes eagerly, with no synthesis: a detected output becomes a leaf, else
  `bound_argument_chooser` picks a heterogeneous argument that constructor just bound to match on next.
- **Synthesis session (`feature_synthesizer`):** nothing is enumerated exhaustively. Operands are the constants (read
  from `constant_store` by reference, never laid out as columns) then the session's heterogeneous features (the
  variables first). `operand_table` alone knows that numbering; both stores also list their positions by type. Each
  step is a two-way choice, stop (option 0) or continue (option 1), so MCTS's branching stays small there; continuing
  chooses a primitive with an application witness (an untried, type-correct operand tuple holding at least one
  heterogeneous feature), then its arguments one at a time, each keeping a witness. A heterogeneous result joins the
  features. The cap is `max_applications` per session; the session also ends when no
  primitive has a witness or once a feature equals the labels.
- **Cost:** a match costs 1 + its scrutinee's size, a leaf its output's size, empty 0. A feature's size is 1 per
  variable and per primitive application, and a constant's own size.
- Design doc: "Partition Model: Structure" (Claude Doc, https://claude.ai/artifact/H1DWSUimtzBz38G4nrhi7e).

Only `core/` exists. There is no CLI or parser yet; tests build their data in C++.

## Layout

```
core/hpp/{infrastructure,value_objects}   headers   (behavior types | plain data)
core/cpp/{infrastructure,value_objects}   .cpp for non-templated types
core/test/{unit,integration}, main.cpp    GTest/GMock
core/test/support/                         user-level code the tests share (the example primitives)
mcts/ googletest/                          git submodules, not ours to edit
debug_assert.hpp                           DEBUG_ASSERT(cond): throws std::logic_error under -DDEBUG
docs/                                      design notes (see docs/continuous-documentation.md)
```

## Architecture

### Layering

```
<solver>_runtime        core/: non-templated facade (.hpp/.cpp): next(), best_model(), best_cost(), infer().
                        Owns exactly one manifest_.
  <solver>_manifest     composition root: names the component types, owns every component by value,
                        wires them through constructor refs
    components          small templated infra types; each depends only on the capabilities it calls
```

- Manifests and runtimes are named after their solver, never after the project. Solver names are short, one word,
  and don't describe the mechanism.
- When you add a solver, add a new manifest + runtime instead of changing an existing one. Duplicate wiring
  across manifests if needed.
- Solvers so far:
  - **Scout** (`scout_*`): one episode, the first option at every choice (so synthesis stops at once and only
    variables or constants become leaf outputs).
  - **Razor** (`razor_*`): UCT over every choice, its parts from the `mcts` submodule wired one by one in
    `razor_manifest`; each episode builds a fresh model and is rewarded with its negated cost. `binary_option_chooser`
    splits every choice into two-way choices (halving the options' indices), so MCTS's branching is at most two.
  - **Hawk** (`hawk_*`): Razor with best-path memory, the `mcts` submodule's `best_path_grafter`: an episode whose
    cost is at most the cheapest so far adds its whole path, rollout included, to the tree, so it is never forgotten
    (CadiaPlayer's single-agent enhancement). Selection and backups stay UCT's.
  - **Kite** (`kite_*`): Hawk with order-tiered choices. A feature's order is 0 for a variable or a constant and one
    more than its highest-order argument for an application. Each primitive argument and each match's feature is
    chosen highest order first: a two-way choice takes the highest order present or descends to the next, then that
    order's options are split as usual. A rollout so builds on the highest-order feature half the time.

### Per-bin columns

A `bin` is columns aligned by row: `inputs[v]` is variable v's column (the data points' own inputs first, then
arguments bound by the matches above), and `labels`. Synthesis applies primitives row-wise to operands
(`operand_applier`); observational equivalence keeps the cheapest feature per column (`cheapest_feature_index`). Raw
data points are never re-read.

### Components (one mechanism each)

- Stores: `schema`, `value_pool`, `constant_store`, `primitive_store`, `cheapest_feature_index` (a session's features),
  `application_log`, `best_model_tracker`, `tree_walker`; `operand_table` is a read-only view over the constant and
  feature stores.
- Synthesis: `constant_seeder`, `synthesis_starter`, `variable_candidate_finder`,
  `witness_detector`, `primitive_option_finder`, `argument_option_finder`, `argument_chooser`,
  `primitive_caller`, `operand_applier`, `app_candidate_finder`, `primitive_applier`,
  `feature_synthesizer`.
- Partition: `leaf_detector`, `output_finder`, `uniform_value_finder`, `bound_argument_chooser`, `bin_splitter`,
  `model_builder`.
- Choices: `first_option_chooser`, `mcts_option_chooser`, `binary_option_chooser`; each chooses an index among
  `option_count` options. `tiered_option_chooser` chooses among options by their orders, and `tiered_argument_chooser`
  and `tiered_feature_chooser` use it for arguments and for a match's feature. MCTS sees them as an `option_range`
  (a count, not a vector).
- Cost and inference: `value_sizer`, `feature_sizer`, `feature_order_measurer`, `model_sizer`, `feature_evaluator`,
  `model_evaluator`.
- Episodes: `model_episode`, `mcts_episode`.

## Build & test

- Iterate with `make core_debug_fast && ./build/core_debug_fast` (`-DDEBUG -O3`). Use `--gtest_filter=...`.
- The makefile `find`s every `.cpp` under `core/cpp`, `core/test` and `mcts/core/cpp`.
  **A new file is picked up automatically, so don't edit the makefile to register it.**
- CI (`.github/workflows/ci.yml`) builds and runs `core_debug_fast`.

## Code style — the rules

The detailed rules live in `.claude/rules/*.md` (copied from Atlas) and load automatically every session.

Core of it in one breath: simplest thing that works. `struct` only. Collaborators are injected **by reference as
template params named per invoked capability**. The only inheritance is the manifest naming trick. No lambdas
(except a genuinely generic `std::visit`), no `std::function` (except `primitive`), no overloads, no default args,
no default member initializers. Every infrastructure struct supports one mechanism: a store, or one job; extra
behavior goes into another struct. A store (passive: holds data) may have several accessors and mutators and is
named as a noun (`_store`, `_pool`, `_index`, `_log`, `_table`). Active infrastructure has exactly one public
function besides its constructor and is named after its job (`_finder`, `_chooser`, `_applier`, `_caller`, ...);
never mix the two, and avoid jargon names that don't say what the struct does. The runtimes are the one exception:
public facades, as in Atlas. Test primitives are function objects whose one public function is `operator()`.

## Conventions observed in the code

- Out-of-line template definitions use a `X_TEMPLATE` / `X` macro pair, `#undef`'d at the end of the header.
- Manifests name every templated component type, submodule ones included, as a nested struct that inherits the
  instantiation (`struct leaf_detector_t : leaf_detector<...> { using leaf_detector<...>::leaf_detector; };`), not
  a `using` alias, so type names stay short instead of nesting. Members are declared in dependency order with their
  names aligned in one column.
- Constructor init lists put the comma first. Value objects use `auto operator<=>(const T&) const = default;` and
  tests build them with designated initializers (`typed_primitive` is the one value object without `operator<=>`,
  since a `std::function` cannot be compared). Ids are aliases in their own headers (`type_id`, `var_id`, ...).
- `std::optional` is checked with `.has_value()` and read with `.value()`. Locals are `const` wherever possible.
- Tests: mocks and the fixture go in an anonymous namespace; NiceMock by default. Once a method has any
  `EXPECT_CALL`, add a catch-all `EXPECT_CALL(..., method(_)).Times(AnyNumber())` before the specific ones.
- Pass by reference wherever possible; inject the collaborator that already holds data instead of copying it into
  another store. A function returns a store's contents by `const&` when its caller only reads them.
- Keep lines to about 120 columns.

## Working agreements

- **Don't create files unless asked.** Use the scratchpad for notes.
- When asked only to write tests, **don't touch non-test source**. Never weaken or delete a test to make it pass.
- New public behavior needs a unit test (one real SUT, everything else mocked). Add an integration test only when
  correctness depends on real components interacting (at least two real components collaborating).
- Keep coverage strong: each component's tests cover its edge cases (empty inputs, ties, order, every branch), not
  just the happy path. Strengthen weak tests without asking.
- Docs: follow `docs/continuous-documentation.md`. Write down only what was stated, and ask instead of guessing.
- Be terse.
- Git: commit messages are short, lowercase, past tense. Branches use `feature/`, `enhancement/`, `bug-fix/`
  prefixes. Default branch is `master`.
