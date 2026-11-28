# AML — project memory for Claude Code

## What this is

AML (abstractive machine learning) learns models of noiseless, functional data that fit every data point exactly
and are minimal in symbolic description length. Fit comes first; among models that fit, cheaper is better.

- **Data:** each data point is an input tuple (bound to variables x0..xn-1) and a label. Values are constructor terms,
  `value{ctor, args}`: an owned tree, compared by structure, where `ctor` is the constructor's `ctor_id` within its
  type. There is no value pool; users build values directly. A value carries no type: columns do (`typed_column`, for
  the inputs and the labels), so comparisons that must respect types compare typed columns.
- **User types are data, never C++ types.** A user type is a `type_def`: its constructors (`ctor_def`), each with its
  argument types. `schema` is the authority on type ids and constructor ids: `declare_type()` gives a type's id, then
  `declare_ctor(type, arg_types)` gives each constructor's `ctor_id`. Declaring types before their constructors allows
  recursive and mutually recursive types. Code that sets up types (tests with real types, the probe) takes ids from
  the schema rather than writing them as literals.
  Core knows no particular user type (no bool, no nat); anything specific to one lives with the user (the tests).
  The exception is the column solvers (Quartz, Obsidian, Amethyst, Granite): their columns hold native C++ values, see below.
- **Primitives and constants:** a `primitive` is a `std::function` from argument values (read in place, as pointers
  into the solver's columns) to a fresh result value; a `typed_primitive` pairs it with its `primitive_type`
  (argument and result types). The constants synthesis may use as arguments are `typed_constant`s the user offers.
  Solvers receive both at construction; nothing is enumerated. They are user code: the ones the tests use live in
  `core/test/support/`.
- **Features:** a variable, a primitive application, or a constant. A feature owns its arguments, so it is a tree. A
  feature is heterogeneous over a bin when its values there are not all equal.
- **Model:** a tree that owns its nodes; each episode builds one, and `best_model_tracker` keeps a copy
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

The library is `core/`. There is no parser yet; tests build their data in C++. `probe/` is a developer's entry point for
watching a solver run by hand (`make probe`, then `./build/probe`), outside the library and its tests; its example is
meant to be swapped.

Everything in `core/` is in `namespace AML` (upper case on purpose), so AML's names read apart from C++'s. Tests say
`using namespace AML;`; user-side code (the probe, `core/test/support/`) writes `AML::` in full.

## Layout

```
core/hpp/{infrastructure,value_objects}   headers   (behavior types | plain data)
core/cpp/{infrastructure,value_objects}   .cpp for non-templated types
core/test/{unit,integration}, main.cpp    GTest/GMock
core/test/support/                         user-level code the tests share (the example primitives)
mcts/ googletest/                          git submodules, not ours to edit
debug_assert.hpp                           DEBUG_ASSERT(cond): throws std::logic_error under -DDEBUG
docs/                                      design notes (see docs/continuous-documentation.md)
probe/                                     developer entry point outside the library (make probe); swappable example
research/                                  experiments and their records; proofs build as make research/<name>
```

## Architecture

### Layering

```
<solver>_runtime        core/: non-templated facade (.hpp/.cpp): next() (returns the episode's model and cost),
                        best_model(), best_cost(), infer(). Only what users normally need; the probe reads a
                        manifest directly for anything else.
                        Owns exactly one manifest_.
  <solver>_manifest     composition root: names the component types, owns every component by value,
                        wires them through constructor refs
    components          small templated infra types; each depends only on the capabilities it calls
```

- Manifests and runtimes are named after their solver, never after the project. Solver names are short, one word,
  and don't describe the mechanism. Bird names are the partition-tree solvers; column solvers never take bird names.
- When you add a solver, add a new manifest + runtime instead of changing an existing one. Duplicate wiring
  across manifests if needed.
- Solvers so far:
  - **Scout** (`scout_*`): one episode, the first option at every choice (so synthesis stops at once and only
    variables or constants become leaf outputs).
  - **Razor** (`razor_*`): UCT over every choice, its parts from the `mcts` submodule wired one by one in
    `razor_manifest`; each episode builds a fresh model and is rewarded with its negated cost. `binary_option_chooser`
    splits every choice into two-way choices (halving the options' indices), so MCTS's branching is at most two.
    `ucb1` scores children by their mean normalized to [0, 1] with the lowest and highest means seen (`value_bounds`,
    owned by the manifest like the visit and value tables), so c is on that scale whatever the costs; above 1 it
    favours exploration.
  - **Hawk** (`hawk_*`): Razor with best-path memory, the `mcts` submodule's `best_path_grafter`: an episode whose
    cost is at most the cheapest so far adds its whole path, rollout included, to the tree, so it is never forgotten
    (CadiaPlayer's single-agent enhancement). Selection and backups stay UCT's.
  - **Kite** (`kite_*`): Razor with order-tiered choices. A feature's order is 0 for a variable or a constant and one
    more than its highest-order argument for an application. Each primitive argument and each match's feature is
    chosen highest order first: a two-way choice takes the highest order present or descends to the next, then that
    order's options are split as usual. A rollout so builds on the highest-order feature half the time.
  - **Osprey** (`osprey_*`): Kite with commitments from the `mcts` submodule. Once the frontier (the root at first)
    gathers `commit_visits` visits since becoming the frontier, the search commits to the choice the best episode so
    far (the one behind the best model) made there, takes it in every later episode and searches only below it
    (`commitment_store`, `committing_policy`, `committed_policy`, `visit_threshold`; the best path from
    `choice_logger`, `episode_choice_log`, `best_path_recorder` and `best_path_store`, read by
    `best_path_child_finder`). Commitments are never undone.
  - **Falcon** (`falcon_*`): Osprey without splitting choices: every choice (a primitive, each argument, a match's
    feature, a funnel's bound argument) is one MCTS choice among all its options, with no two-way splitting and no
    order tiers; stopping or continuing synthesis stays a two-way choice. Commits follow the best episode's path.
  - **Heron** (`heron_*`): Falcon's flat choices without commitments, selecting with the `mcts` submodule's
    `focus_ucb1`: at a node with N visits, the first child in index order whose normalized mean is within
    A·N^(−r) of the best gets a constant bonus β (`focus_width`, `focus_rate`, `focus_bonus`), so interchangeable
    choices stop splitting the visits evenly while the shrinking band lets any real difference win.
  - **Kestrel** (`kestrel_*`): Kite with mixmax selection from the `mcts` submodule: each backup adds the reward to a
    node's sum and raises its best (`max_value_update`, `paired_value_update`), and selection reads a node as
    (1 − w)·mean + w·best (`mixmax_value_view`, `best_weight`), so a deep good path among poor siblings is not
    averaged away.
  - **Merlin** (`merlin_*`): flat choices with mixmax: in the search tree every choice is one MCTS choice among all
    its options (no two-way splitting; stopping or continuing synthesis stays two-way), as in Heron, with Kestrel's
    mixmax selection in place of focus ucb1. In rollouts, an argument and a match's feature are chosen by order tiers
    as in Kite (`rollout_tiered_option_chooser`), so random completions build on high-order features.
  - **Swift** (`swift_*`): Merlin with an argument and a match's feature chosen by quadrant. An operand is fresh until
    the session uses it as an argument (`application_log::is_used`) and consumed after, and made if synthesized or
    given if an input, a bound argument or a constant: made and fresh (a chain's tip), given and fresh, made and
    consumed, given and consumed (`operand_quadrant_measurer`, `feature_quadrant_measurer`). In the tree a choice is
    one flat choice among the quadrants holding options, made and fresh first, then one within the quadrant
    (`quadrant_option_chooser`); in rollouts fresh is taken with probability `fresh_weight` and made with
    `made_weight` (0.5 is no preference; `weighted_quadrant_chooser`, `fraction_drawer`, switched by
    `rollout_switching_option_chooser`).
  - **Wren** (`wren_*`): Swift with every choice among many options split into two-way choices in the search tree
    (`binary_option_chooser`, as in Razor): the primitive, an argument or a match's feature within its quadrant, and a
    funnel's bound argument. The quadrant stays one flat choice among at most four, so the tree's branching is at most
    four whatever the number of primitives or features, and how soon the search exploits no longer grows with them.
    Rollouts choose within a quadrant as Swift's do.
  - **Raven** (`raven_*`): Wren whose matches pass what they built down as variables. Below a match a child's
    variables are its parent's, its constructor's arguments, then the scrutinee and every primitive application inside
    it, each once in preorder (`scrutinee_piece_finder`), evaluated once over the parent's rows and selected per child
    (`inheriting_bin_splitter`, `feature_column_evaluator`). A piece is paid for once in the match's scrutinee and costs
    1 per use below, like any variable; a piece uniform over a bin stays among its variables (so numbering never
    depends on rows) but is never offered. Inference binds the same variables (`inheriting_model_evaluator`).
  - **Jay** (`jay_*`): Raven with flat choices, as in Swift: in the search tree the primitive, an argument or a match's
    feature within its quadrant, and a funnel's bound argument are each one choice among all their options, so every
    option gets its own child and is tried before any is tried twice; the quadrant stays one flat choice. Matches pass
    what they built down as variables, as in Raven.
- Column solvers (no partition tree; the model is a `table_model`: the graph of features it reads, their training
  columns, and a lookup from each distinct tuple of their classes to its label, with no prediction for a value or a
  tuple no training row held). Their values are native C++ values, not `value` trees: a type is a `std::type_index`,
  and a `native_type` (`native_type::of<T, Hash>()`) erases the operations a column of T needs (classify, hash, fill,
  compare and read cells), a column being a `std::vector<T>` in a `std::any`. A `native_primitive`
  (`native_primitive::of(std::function<Ret(Args...)>)`) is a native function lifted to columns
  (`lifted_primitive`), unwrapping each operand once; an operand is a column or a scalar read at every row
  (`operand_data`). Data is a `native_bin` of `native_column`s; constants are `native_constant`s:
  **Quartz** (`quartz_*`): best-first search over sets of features that are functions of the labels (each distinct row
    of the set's values has one label). Constants never vary over rows, so they live in a `constant_graph` of their own,
    one value each: the offered constants are atoms, and a constant may be a primitive applied to other constants. As
    with features, the graph keeps only origins and types; values live in a `constant_cache` within
    `constant_cache_bytes`, recomputed from their origins when evicted (`constant_reader`). A feature's application
    takes `operand`s, each a feature or a constant. A set's cost is the number of features in its ancestry plus the
    number of constants those features reach, with their own ancestry (shared ancestors once), plus its distinct rows.
    Visiting the cheapest set discards it and pushes one child per single move: every legal drop
    (`droppable_member_finder`, exact by prefix and suffix row keys) and up to `adds_per_feature` sampled additions per
    feature in its ancestry. An application over constants alone can never vary, so every sampled application has a
    feature operand: on each visit the primitives taking a type the ancestry holds features of are eligible
    (`argument_type_index`, `eligible_primitive_finder`); a primitive is drawn uniformly among them, then each argument
    among the ancestry's features and the constants of its type, all drawn again while none is a feature. A set's key is
    the exclusive or of its members' random keys. A set is queued again only once its ancestry is smaller than every
    time it was queued, as when a cheaper origin is found for a feature beneath it; its distinct rows never change, so
    otherwise it is a repeat. The queue holds at most `queue_capacity` sets, dropping the costliest. Features are never
    deleted (`feature_graph`); their columns live in a `column_cache` within `column_cache_bytes`, recomputed from their
    origins when evicted. A derived column equal to a feature's merges into it, keeping the cheaper origin; a column
    holding one value everywhere is discarded.
  **Obsidian** (`obsidian_*`): Quartz's sets, graphs, caches and cost searched by UCT in place of best-first, since
    best-first only exploits and buries early sets under their descendants. Each episode walks from the root set (the
    varied inputs) by up to `max_moves` moves and is rewarded with the negated cost of the cheapest set it passed
    through (normalized as in Razor); a node backs up the best reward through it, not the mean, and selection reads
    that best (`max_value_update`, `mixmax_value_view` with the best weighted fully). A move is one drop or one addition (`set_mover`), made of flat MCTS choices with
    nothing capped: drop or add when both are offered, then a droppable member, or a fillable primitive
    (`fillable_primitive_finder`: eligible, and every argument has a pool feature or a constant) and each argument left
    to right among its type's ancestry features then constants; while no feature is chosen, the last argument a
    feature can fill offers features alone (`application_chooser`). An addition deriving no feature or a member leaves
    the set as it was. Repeats are allowed: the feature graph persists across and within episodes, so a cheaper origin
    found anywhere prices every later set. Rollouts choose uniformly.
  **Amethyst** (`amethyst_*`): Obsidian with Hawk's best-path memory and Razor's two-way choices. An episode whose
    reward is at least the best so far adds its whole path, rollout included, to the tree (`best_path_grafter`), and
    nodes back up the best reward, so a cheap set a rollout reached is never re-found move by move. Every choice is split
    into two-way choices (`binary_option_chooser`), so a node has at most one untried sibling and a grafted path is
    exploited after at most one detour per level.
  **Granite** (`granite_*`): Quartz with Type-GBFS's exploration (Xie, Mueller, Holte and Imai, AAAI 2014), the
    standard fix for greedy best-first search stuck on plateaus. Every queued set also goes into the bucket of its
    stratum, the "type" of that paper: its ancestry size and its distinct rows (`set_stratum`, `stratum_bucket_store`,
    `stratified_child_pusher`). Visits alternate (`alternating_set_popper`) between Quartz's cheapest set and an
    exploring pick, a bucket uniformly at random then a set uniformly at random within it (`random_stratum_popper`);
    a set popped from one is skipped when the other gives it again (`unvisited_set_popper`, `visited_set_store`, keyed
    by set key and cost). The buckets have no capacity, so a set the queue dropped can still be visited.

### Per-bin columns

A `bin` is columns aligned by row: `inputs[v]` is variable v's column (the data points' own inputs first, then
arguments bound by the matches above), and `labels`. Users give their data as a `bin` of dense `typed_column`s; model
building reads it as a `bin_view`, whose columns are `column_view`s (shared cells and the rows they select), and a
runtime views its training data once. So splitting a bin (`bin_splitter`) shares its parent's cells and only selects
rows (a constructor's bound arguments are the only new cells), and a variable's candidate shares its bin's column.
Synthesis applies primitives row-wise to operands (`operand_applier`); observational equivalence keeps the cheapest
feature per column (`cheapest_feature_index`). Raw data points are never re-read.

### Components (one mechanism each)

- Stores: `schema`, `constant_store`, `primitive_store`, `cheapest_feature_index` (a session's features),
  `application_log`, `best_model_tracker`, `tree_walker`; `operand_table` is a read-only view over the constant and
  feature stores.
- Synthesis: `constant_seeder`, `synthesis_starter`, `variable_candidate_finder`,
  `witness_detector`, `primitive_option_finder`, `argument_option_finder`, `argument_chooser`,
  `primitive_caller`, `operand_applier`, `app_candidate_finder`, `primitive_applier`,
  `feature_synthesizer`.
- Partition: `leaf_detector`, `output_finder`, `uniform_value_finder`, `bound_argument_chooser`, `bin_splitter`,
  `inheriting_bin_splitter`, `scrutinee_piece_finder`, `model_builder`.
- Choices: `first_option_chooser`, `mcts_option_chooser`, `binary_option_chooser`; each chooses an index among
  `option_count` options. `tiered_option_chooser` chooses among options by their orders, and `tiered_argument_chooser`
  and `tiered_feature_chooser` use it for arguments and for a match's feature. MCTS sees them as an `option_range`
  (a count, not a vector).
- Cost and inference: `value_sizer`, `feature_sizer`, `feature_order_measurer`, `model_sizer`, `feature_evaluator`,
  `feature_column_evaluator`, `model_evaluator`, `inheriting_model_evaluator`.
- Episodes: `model_episode`, `mcts_episode`.
- Quartz:
  - Native types: `native_classifier`, `native_column_hasher`, `native_column_filler`, `native_cell_comparer`,
    `native_cell_reader`, `lifted_primitive`, `operand_cell_reader` (the operations `native_type` and
    `native_primitive` erase).
  - Stores: `native_type_store`, `native_primitive_store`, `argument_type_index`, `feature_graph`, `constant_graph`,
    `derivation_index`, `column_hash_index`, `column_cache`, `constant_cache`, `set_key_store`, `set_queue`,
    `best_table_tracker`.
  - Columns: `column_computer`, `cached_column_maker`, `column_byte_measurer`, `column_reader`,
    `uniform_column_detector`.
  - Constants: `constant_computer`, `constant_byte_measurer`, `constant_reader`.
  - Features: `key_drawer`, `feature_recorder`, `equal_feature_finder`, `origin_replacer`, `feature_deriver`,
    `input_feature_seeder`, `constant_atom_seeder`.
  - Sets: `ancestry_finder`, `constant_ancestry_finder`, `ancestry_sizer`, `row_key_numberer`,
    `distinct_row_counter`, `droppable_member_finder`, `set_key_measurer`, `child_pusher`,
    `operand_pool_finder`, `eligible_primitive_finder`, `derivation_sampler`, `dropping_expander`, `adding_expander`,
    `cheapest_set_visitor`.
  - Models: `model_graph_builder`, `table_model_builder`, `graph_row_evaluator`, `value_class_finder`,
    `table_model_evaluator`.
- Granite (Quartz's components plus): `set_stratum`, `stratum_bucket_store`, `visited_set_store`,
  `stratified_child_pusher`, `random_stratum_popper`, `alternating_set_popper`, `unvisited_set_popper`.
- Obsidian and Amethyst (Quartz's components but the queue and its expansion): `fillable_primitive_finder`, `application_chooser`,
  `set_mover`, `set_cost_measurer`, `set_walk_episode`, `mcts_walk_episode`.

## Build & test

- Iterate with `make core_debug_fast && ./build/core_debug_fast` (`-DDEBUG -O3`). Use `--gtest_filter=...`.
- The makefile `find`s every `.cpp` under `core/cpp`, `core/test` and `mcts/core/cpp`.
  **A new file is picked up automatically, so don't edit the makefile to register it.**
- CI (`.github/workflows/ci.yml`) builds and runs `core_debug_fast`.
- A research proof (`research/<name>/`, e.g. `kestrel_parity`, `swift_parity`, `swift_scaled_square_of_sum`,
  `swift_reused_product`) is one recorded run with its settings fixed in its source; a new proof stops after a fixed
  number of episodes, not seconds, so it does the same work on any machine; `make research/<name>` builds it into
  `build/research/<name>`. Never edit a proof's settings; a new experiment gets a new proof and a new
  `research/<name>` target.

## Code style — the rules

The detailed rules live in `.claude/rules/*.md` (copied from Atlas) and load automatically every session.

Core of it in one breath: simplest thing that works. `struct` only. Collaborators are injected **by reference as
template params named per invoked capability**. The only inheritance is the manifest naming trick. No lambdas
(except a genuinely generic `std::visit`), no `std::function` (except `primitive`, and the native type erasure:
`native_type`'s operations and `native_primitive::apply`), no overloads, no default args, no default member
initializers. Every infrastructure struct supports one mechanism: a store, or one job; extra
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
  tests build them with designated initializers. The exceptions: `typed_primitive` has no comparison, since a
  `std::function` cannot be compared; `column_view` defines `operator==` by the values it reads, not by the cells and
  rows it shares, so `bin_view` and `candidate`, which hold views, default `operator==` only. Ids are aliases in their
  own headers (`type_id`, `var_id`, ...).
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
