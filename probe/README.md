# Probe

A developer's entry point for watching a solver run by hand, outside the core library and its tests. `probe/main.cpp`
holds one example at a time; swap its code for whatever you want to watch. It links the release core library
(`build/libaml_core.a`) and may use the test support primitives (`core/test/support/`).

```bash
make probe
./build/probe
```

It takes no arguments. Settings live as constants at the top of `probe/main.cpp`; change them there and rebuild.

## Current example: Falcon on odd-length lists

Trains **Falcon**, through `falcon_manifest` directly, to label boolean lists true when their length is odd. The
training set is every boolean list of length 0 to `k_max_training_length` (127 lists at 6). The elements play no part
in the label. Synthesis may use general-purpose list primitives (`probe/list_primitives.hpp`: `is_nil`, `head`,
`tail`) and `not` and `xor`, with the constants F, T and nil. It runs until a time limit, a memory limit or Ctrl-C,
then prints the best model, and for every length up to `k_max_unseen_length` how many lists the best model gets right,
wrong or leaves unanswered. Lengths above the training ones are unseen.

Its settings, at the top of `probe/main.cpp`: `k_max_training_length` (6), `k_max_unseen_length` (9),
`k_max_applications` (4), `k_exploration_constant` (4, on `ucb1`'s normalized scale), `k_seed` (1), `k_commit_visits`
(0: never commits), `k_seconds` (0 runs until Ctrl-C), `k_report_seconds` (10), `k_max_rss_mb` (4,000) and
`k_model_lines` (60).

What it prints:

- `new best COST at Ts (episode E): ROOT` whenever the best model improves, with what its root does.
- `commitment K at Ts (episode E)` whenever the search commits one more choice.
- Every `k_report_seconds`, a progress line: episodes and episodes per second, resident memory, the best cost, the
  commitments so far, and the mean cost of the episodes since the last line.
- At the end, the best model as a tree (a list match's `case 0` is nil and `case 1` is cons, binding the head and
  tail as the next variables; a boolean match's `case 0` is F and `case 1` is T), then the score by length.

A model has no recursion, so with these primitives it can only fit lengths up to a fixed depth: in 60 seconds Falcon
finds a cost-18 model that fits every training list, and on unseen lengths it is right only where its last test
happens to agree with parity. A primitive that recurses (a fold over the list) would let a single leaf generalize.

The previous example, Falcon on n-bit parity, is in git history.
