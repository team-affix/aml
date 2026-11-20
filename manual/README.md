# Manual runs

Programs for watching a solver work, outside the core library and its tests. They link the release core library
(`build/libaml_core.a`) and the test support primitives.

## Parity

`manual/parity.cpp` trains **Falcon** on n-bit parity: the training set is every row of the truth table, the label
is the parity of the bits, and synthesis may use xor, and, or and not over the inputs and the constants F and T. It
runs until a time limit, a memory limit or Ctrl-C, then prints a summary and the best model found.

```bash
make manual
./build/manual_parity --seconds=600
```

Options (defaults in brackets):

| Option | Meaning |
|---|---|
| `--bits=N` [10] | parity over N input bits; the training set has 2^N rows |
| `--max-applications=N` [9] | synthesis applications per session |
| `--c=X` [4] | exploration constant, on `ucb1`'s normalized [0, 1] scale; above 1 favours exploration |
| `--seed=N` [1] | random seed |
| `--commit-visits=N` [52000] | visits the frontier gathers before the search commits along the best path; 0 never commits |
| `--seconds=X` [0] | stop after X seconds; 0 runs until Ctrl-C |
| `--report-seconds=X` [10] | seconds between progress lines |
| `--max-rss-mb=N` [4000] | stop once resident memory passes N MB (checked every 1,000 episodes) |
| `--model-lines=N` [40] | lines of the best model to print at the end |

What it prints:

- `new best COST at Ts (episode E): ROOT` whenever the best model improves, with what its root does (`match FEATURE`
  or `leaf FEATURE`).
- `commitment K at Ts (episode E)` whenever the search commits one more choice.
- Every `--report-seconds`, a progress line: episodes and episodes per second, resident memory, the best cost, the
  commitments so far, and for the episodes since the last line their mean cost and how many xors the feature their
  root matched on held (`0:` none, `5+:` five or more).
- At the end, the same counts for the whole run and the last window, and the best model as a tree (`T ->` and `F ->`
  are a match's branches).

At about 400 episodes a second on 10 bits, the default `--commit-visits=52000` commits roughly every two minutes.
Memory grows by roughly 5 MB a minute without commitments' pruning of the search, so runs of an hour or more stay well
under the default limit.
