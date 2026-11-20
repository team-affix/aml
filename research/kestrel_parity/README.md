# Kestrel on 10-bit parity

How many nested xors does a search put at the root of its 10-bit parity models? A model whose root matches on the
xor of all ten bits, or outputs it, is parity itself; every bit left out has to be handled below the root, at a cost
that doubles with each one. Plain UCT, focus UCB1 (Heron) and order-tiered choices (Kite) held at most 2 xors at the
root over 25 minutes. Kestrel, Kite with mixmax selection, builds them up to 8.

## Setup

- Data: the full truth table of 10 bits (1024 rows); the inputs x0..x9 are the bits, and the label is their parity.
- Primitives: xor, and, or, not. Constants: false, true. `max_applications` = 9.
- Solver: Kestrel (`kestrel_runtime`), exploration constant c = 5 on `ucb1`'s normalized scale, best weight w = 1 (a
  node is read by its best episode), seed 1, 3000 seconds.
- Cost is the model's size (lower is better; the reward is its negation). The smallest exact model is a single leaf
  outputting the xor of all ten bits: 9 xors over 10 variables, cost 19.

## Result

Seed 1, `kestrel_parity 5 1 1 3000`, about 400 episodes per second (1,258,909 episodes):

| Minute | Best cost | Xors at the best model's root | Episodes in the last minute by root xors |
|---|---|---|---|
| 4 | 1048 | 3 | 3: 55% |
| 8 | 765 | 5 | 4: 42%, 5: 12% |
| 12 | 558 | 5 | 5: 73% |
| 20 | 119 | 6 | 6: 77% |
| 24 | 103 | 7 | 6: 45%, 10+: 15% |
| 28 | 26 | 7 | 6: 37%, 7: 31% |
| 32 | 23 | 8 | 6: 56%, 7: 21% |
| 44 | 23 | 8 | 7: 49%, 6: 26% |
| 48 | **21** (found at minute 47) | **8** | 7: 57%, 6: 22% |

The best model, found at episode 1,162,451 (the log records its root; its branches are the only ones its cost and an
exact fit allow):

```
match xor(xor(x5, xor(xor(xor(x3, xor(xor(x1, xor(x6, x4)), x0)), x2), x9)), x8)
  F -> leaf x7
  T -> leaf not(x7)
```

Its root holds 9 of the 10 bits (8 xors); the last bit, x7, is handled by its branches. Cost 21: the match 1, its
feature 17, the branches 1 and 2. It fits every row, two above the smallest exact model.

The episodes were still moving to deeper roots when the run ended (7 xors in 57% of the last minute's episodes, against
under 1% at minute 20), so the search had not converged. Resident memory grew to 347 MB.

`c5_w1_seed1_3000s.log` is the run's log: every new best model with the episode that found it and its root, and a
progress line each minute. It was printed by the experiment driver this program was cut down from; the episodes are
the same. Rerunning `kestrel_parity 5 1 1 300` gave the same 18 new best models, at the same episodes, as the log's
first five minutes.

### Other settings (seed 1, 25 minutes, same data and primitives)

| Solver | c | w | Root xors in the last minute's episodes | Best cost |
|---|---|---|---|---|
| Heron (β 0.25, A 0.01, r 0.5) | 4 | | 1 (100%) | 1667 |
| Kestrel (5-minute run) | 2 | 1 | 3 (100%, from minute 2) | 771 |
| Kestrel | 3 | 1 | 5 (99.9%; 100% from minute 9) | 357 |
| Kestrel | 4 | 1 | 5 (100%, from minute 12) | 274 |
| Kestrel | 5 | 1 | 6 (47%), still moving | 103 |

A larger c settles later and deeper.

The whole investigation that led here, with every run and its log, is in [investigation.md](investigation.md).

## Why mixmax

This is an interpretation of the measurements in [investigation.md](investigation.md), not something tested on its
own. Building a deeper root takes a chain of choices: continue synthesis, pick xor, pick the chain so far, pick an unused
bit. Under plain UCT a node is read by its mean, so "continue synthesis" after a chain averages the one good extension
with the many poor ones around it and loses to stopping, and the good extension is never visited enough to be found.
Mixmax reads a node as (1 − w) · mean + w · best, so once one episode extends the chain, every node on its path is
worth that episode, and the search follows it.

## Reproducing

From the repository root:

```bash
make core
```

```bash
g++ -std=c++20 -O3 -I. -Icore/hpp -Imcts/core/hpp -Icore/test research/kestrel_parity/kestrel_parity.cpp probe/bool_xor.cpp core/test/support/bool_and.cpp core/test/support/bool_or.cpp core/test/support/bool_not.cpp -Lbuild -laml_core -o build/kestrel_parity
```

```bash
build/kestrel_parity 5 1 1 3000
```
