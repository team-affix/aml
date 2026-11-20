# Probe

A developer's entry point for watching a solver run by hand, outside the core library and its tests. `probe/main.cpp`
holds one example at a time; swap its code for whatever you want to watch. It links the release core library
(`build/libaml_core.a`) and may use the test support primitives (`core/test/support/`).

```bash
make probe
./build/probe
```

It takes no arguments. Settings live as constants at the top of `probe/main.cpp`; change them there and rebuild.

## Current example: Falcon on parity

Trains **Falcon** on n-bit parity, through `falcon_manifest` directly (it also watches the commitments, which no runtime
exposes): the training set is every row of the truth table, the label is the parity of the
bits, and synthesis may use xor (`probe/bool_xor.hpp`), and, or and not over the inputs and the constants F and T. It
runs until a time limit, a memory limit or Ctrl-C, then prints a summary and the best model found.

Its settings, at the top of `probe/main.cpp`: `k_bits` (10), `k_max_applications` (9), `k_exploration_constant` (4,
on `ucb1`'s normalized scale), `k_seed` (1), `k_commit_visits` (52,000; 0 never commits), `k_seconds` (0 runs until
Ctrl-C), `k_report_seconds` (10), `k_max_rss_mb` (4,000) and `k_model_lines` (40).

What it prints:

- `new best COST at Ts (episode E): ROOT` whenever the best model improves, with what its root does (`match FEATURE`
  or `leaf FEATURE`).
- `commitment K at Ts (episode E)` whenever the search commits one more choice.
- Every `k_report_seconds`, a progress line: episodes and episodes per second, resident memory, the best cost, the
  commitments so far, and for the episodes since the last line their mean cost and how many xors the feature their
  root matched on held (`0:` none, `5+:` five or more).
- At the end, the same counts for the whole run and since the last progress line, and the best model as a tree
  (`T ->` and `F ->` are a match's branches).

At about 400 episodes a second on 10 bits, `k_commit_visits` = 52,000 commits roughly every two minutes.
Memory grows by about 5 MB a minute, so runs of an hour or more stay well under the default limit.
