# How the root xors went from 1 to 8: the investigation

This is the record of how AML's search on 10-bit parity went from a model whose root holds 1 xor to one whose root
holds 8, over 19 and 20 November 2026. It says what was run, what each run printed, and what was concluded from it.
Measurements are quoted from the logs in `logs/` and `c5_w1_seed1_3000s.log`; statements marked **Interpretation**
are explanations of the measurements that were not tested directly. The final run and how to reproduce it are in
[README.md](README.md).

## The question

Parity of 10 bits has an exact model of cost 19: one leaf outputting the xor of all ten bits (9 xors). A model that
matches its root on the xor of only some of the bits must handle every remaining bit below the root, and each bit
left out doubles what the branches have to tell apart. So the number of xors in the root's feature is a direct
measure of how far the search has got. The goal was the most xors at the root after 25 minutes, with 9 the best case.

Conditions shared by every run here, unless a run says otherwise:

- Data: every row of the 10-bit truth table (1024 rows); inputs x0..x9, label their parity.
- Primitives xor, and, or, not (`xaon`); "xor only" runs drop the other three. Constants false and true.
- `max_applications` = 9, seed 1, one run at a time on one machine.
- "Root xors" counts the xor applications in the feature the root matches on (or outputs, for a leaf root). Progress
  lines give the share of the episodes since the previous line by root xors (0 to 9, then 10 or more), so they show
  where the search is spending its episodes, not only what the best model is.

All runs were printed by one experiment driver built against the library, which selects the solver by name. Its
solvers fall in two groups:

- **In the repository:** Razor, Kite, Heron, and from this investigation Kestrel.
- **Experimental variants, never committed** (each a copy of a repository manifest with one change):
  - **Wren**: Heron where a bin is matched on the newest feature its session synthesized, if it synthesized any.
  - **Root-only Kite** (`rootkite`): Kite whose search tree stops at the root's match: once the root's feature is
    chosen, every later choice in the episode is a rollout choice.
  - **Root-only Kite with grafting** (`roothawk`) and **Kite with grafting** (`kitehawk`): those two with the `mcts`
    submodule's `best_path_grafter` (Hawk's best-path memory).
  - **Root-only Kite with mixmax** (`rootmix`) and **Kite with mixmax** (`kitemix`): with the selection Kestrel now
    uses. `kitemix` is wired as Kestrel is.

## 1. Baseline: Heron holds 1 xor

Heron (flat choices with focus UCB1), c = 4, β = 0.25, 25 minutes:

| Focus band | Best cost | Best model's root | Episodes in the last window |
|---|---|---|---|
| A = 0.1, r = 0.1 | 1847 | `match xor(x7, xor(x5, x1))` (2 xors) | 0 xors (100%) |
| A = 0.01, r = 0.5 | 1667 | `match xor(x8, x0)` (1 xor) | 1 xor (100%; 93.6% over the whole run) |

Logs: `logs/1_baselines/`.

## 2. One-minute screens across solvers

60-second runs of Heron, Wren, Kite and Razor at several c, with all four primitives and with xor only
(`logs/2_screens/`). What they showed:

- At c = 0.5 to 1, every solver settled on a 1-xor root within the minute (for example Heron c = 1: 99.5%; Wren
  c = 1: 100%; Kite c = 1: 95.4%).
- At c = 4 the episodes stayed spread out: Heron c = 4 spent 87% of its last window on roots with no xor.
- Kite built the deepest best models: 4 xors with xor only at c = 1 (best 903 at 8 s), 3 xors with all four
  primitives at c = 2 and c = 4. None held them; the episodes went back to shallower roots.
- With xor only, every best cost dropped sharply (Wren c = 1: 333), but the roots stayed shallow (Wren c = 1: 1 xor,
  100%).

## 3. A search tree over the root bin only

Root-only Kite searches only the choices that build the root's feature; everything below the root is a rollout.
(`logs/3_root_only_and_grafting/`.)

- 60 seconds: c = 0.5 settled on 2 xors (96.1%); with xor only, on 3 xors (95.3%, best 800). c = 1 reached a 3-xor
  best (1454).
- 5 minutes at c = 0.5 and at c = 1: 2 xors (100% and 99.4%).
- **25 minutes at c = 1: 2 xors in 95.8% of all 533,793 episodes, best 1242** (`xor(xor(x0, x1), x5)`). The 3-xor
  root was never kept.

This was the deepest root held for 25 minutes before mixmax.

## 4. Why it stopped at 2

Three measurements:

- **The rollout reward does favour deeper roots.** With the root fixed to the xor of the first k + 1 bits and
  everything below it random, the mean cost falls with every added xor (see `research/xor_ladder/README.md`: about
  −107, −102, −145 for the first three xors, growing to −422 at the eighth).
- **"Continue synthesis" lost to "stop".** A dump of the most visited path of a root-only Kite run at c = 1
  (`logs/3_root_only_and_grafting/rootkite_c1_path_dump.log`, 51,713 episodes) shows, at depth 19, after the 2-xor
  chain was built: stop, 11,197 visits, mean −2020; continue, 657 visits, mean −2110.
- **The gap is small on UCB1's scale.** `ucb1` normalizes means to [0, 1] with the lowest and highest node means seen
  anywhere in the tree. After 2 minutes of root-only Kite at c = 1 (not logged; printed by the driver at the end of
  the run), a mean of −2020 normalized to 0.428, −2110 to 0.337 and −1880 to 0.570. The 90-point gap between
  continuing and stopping is 0.09 on that scale, and the exploration term at c = 1 for a node with a few hundred
  visits is larger than that.

**Interpretation.** Building the third xor takes about ten two-way choices under "continue": pick xor, pick the chain
as an argument, pick an unused bit, then match on the result. Read by its mean, "continue" averages that one good path
with all the poor continuations around it, so it scores below "stop", and the good path, deep under it, does not get
enough visits for its own mean to rise.

## 5. Best-path grafting did not help

Hawk's mechanism keeps the whole path of any episode at least as good as the best so far in the tree. Over 90
seconds, root-only Kite with grafting settled on 2 xors (c = 0.5: 99.6%; c = 1: 85.8%), and Kite with grafting on 1
xor (c = 1: 98.0%); Kite with grafting at c = 2 found a 4-xor best (1735) but spent 51% of its last window on 1 xor.

**Interpretation.** Grafting keeps the path in the tree, but each node on it is still read by its mean, which the
poor siblings still pull down.

## 6. Mixmax

Mixmax reads a node as (1 − w) · mean + w · best, where best is the best episode reward backed up through it. It was
added to the `mcts` submodule as `mixmax_value_manifest`, with `max_value_update` (restored from an earlier commit),
`paired_value_update` and `mixmax_value_view` (team-affix/mcts#8). Its `mixmax_needle` integration test plays a game
where one first move pays 0.6 and the other pays 1 only along one path two moves further down: at c = 4 over 20,000
episodes, plain UCT finds that path but keeps 6 visits on it; mixmax at w = 0.5 puts 16,052 there.

Root-only Kite with mixmax (`logs/4_mixmax/`):

| c | w | 60 s, seed 1 | seed 2 | seed 3 |
|---|---|---|---|---|
| 1 | 0.5 | 2 xors (100%) | | |
| 1 | 0.75 | 4 xors (99.7%) | 2 xors (99.6%) | 1 xor (99.9%) |
| 1 | 1 | 1 xor (100%) | | |
| 2 | 0.5 | spread (2 xors: 43%) | | |
| 2 | 0.75 | spread (1 xor: 50%) | | |
| 2 | 1 | 4 xors (95.9%) | 3 xors (83.8%) | 3 xors (92.7%) |
| 3 | 1 | spread | spread | spread |
| 4 | 0.5 to 1 | spread, mostly 0 to 1 xor | | |

c = 2, w = 1 was the only setting that went deep on all three seeds. Over 25 minutes at seed 1 it held **4 xors in
98.5 to 100% of every window from minute 2 on, best 825**, then gained no more xors.

5-minute runs (`logs/4_mixmax/rootmix_kitemix_xor_only_5min.log`):

| Variant | c | w | Primitives | Last minute | Best cost |
|---|---|---|---|---|---|
| Root-only | 2 | 0.9 | all four | 4 xors (99.9%) | 904 |
| Root-only | 2.5 | 1 | all four | 3 xors (99.2%) | 1162 |
| Root-only | 3 | 1 | all four | 4 xors (95.7%), still moving | 833 |
| Whole tree (as Kestrel) | 2 | 1 | all four | 3 xors (100%) | 771 |
| Root-only | 2 | 1 | xor only | 5 xors (99.9%) | 358 |

The whole-tree variant was as deep as the root-only ones, with the lowest cost, so the root-only restriction was not
needed once mixmax was in place. It was added to the repository as **Kestrel** (`kestrel_manifest`,
`kestrel_runtime`, `kestrel_options` with `best_weight`).

## 7. Kestrel, raising c

Kestrel, w = 1, all four primitives, seed 1, 25 minutes each (`logs/5_kestrel/`):

| c | Last window at 25 min | Settled | Best cost |
|---|---|---|---|
| 3 | 5 xors (99.9%) | from minute 9 | 357 |
| 4 | 5 xors (100%) | from minute 12 | 274 |
| 5 | 6 xors (47%), 7 xors (8%) | no | 103 (7 xors) |

A larger c settled later and on a deeper root. At c = 5 the episodes were still moving at 25 minutes, so it was run
for 50 minutes (`c5_w1_seed1_3000s.log`): the best model reached 8 xors at minute 29 (cost 23) and cost 21 at minute
47, and 7-xor roots rose from under 1% of the episodes at minute 20 to 57% at minute 48. The details are in
[README.md](README.md).

**Interpretation.** With w = 1, a node is worth the best episode ever backed up through it. The more episodes a
root has had, the better its best tends to be, which may be why a search settles on the first root that gets many
episodes; a larger c spreads the episodes for longer before that happens.

## What is not known

- Every result is from seed 1 except the three-seed screens in section 6. Kestrel's 25- and 50-minute results have
  not been repeated on other seeds.
- The c = 5 run had not converged at 50 minutes; where it settles, and whether a larger c goes further, is open.
- Resident memory grew with the tree: 171 MB at 25 minutes (c = 3), 347 MB at 50 minutes (c = 5).
- Wren, root-only Kite and the grafting variants exist only in their logs; their code was not committed.

## Logs

| Folder | What |
|---|---|
| `logs/1_baselines/` | Heron, 25 minutes, two focus bands |
| `logs/2_screens/` | 60-second screens of Heron, Wren, Kite and Razor; `_xor_only` runs use xor alone |
| `logs/3_root_only_and_grafting/` | root-only Kite (60 s, 5 min, 25 min), its path dump, and the grafting variants (90 s) |
| `logs/4_mixmax/` | root-only Kite with mixmax: the c and w sweep, the seeds, its 25-minute run, and the 5-minute runs |
| `logs/5_kestrel/` | Kestrel at c = 3, 4 and 5 for 25 minutes |
| `c5_w1_seed1_3000s.log` | Kestrel at c = 5 for 50 minutes: the run in README.md |
