# Swift on 10-bit parity

Swift finds the full parity function: a single leaf outputting the xor of all ten bits, cost 19, the smallest exact
model. Earlier solvers reached at most 8 xors at the root (Kestrel, cost 21, after 29 minutes).

## Setup

- Data: the full truth table of 10 bits (1024 rows); the inputs x0..x9 are the bits, and the label is their parity.
- Primitives: xor, and, or, not. Constants: false, true. `max_applications` = 9.
- Solver: Swift (`swift_runtime`), exploration constant c = 2 on `ucb1`'s normalized scale, best weight w = 1,
  `fresh_weight` 0.75, `made_weight` 0.5, seed 2, 25,000 episodes.
- The run stops after a fixed number of episodes, not seconds, so it does the same work on any machine.

## Result

`build/research/swift_parity`, about 540 episodes per second:

| Episode | Best cost | Xors at the best model's root |
|---|---|---|
| 5,000 | 1786 | |
| 10,000 | 1334 | |
| 15,000 | 1129 | |
| 19,931 | 228 | 6 |
| 21,311 | 202 | 8 |
| **23,550** | **19** | **9 (the whole function, as a leaf)** |

The best model, found at episode 23,550 (42.9 seconds here):

```
leaf xor(x9, xor(x2, xor(x6, xor(xor(x3, xor(xor(xor(x1, xor(x4, x5)), x7), x8)), x0))))
```

`proof_check.log` is this program's run. Its new best models are those of the experiment it was cut down from, at the
same episodes.

## What Swift is

Swift is Merlin (flat choices with mixmax selection, w = 1) with an argument and a match's feature chosen by quadrant.
An operand is made if synthesized or given if an input, a bound argument or a constant, and fresh until the session uses
it as an argument or consumed after. In an xor chain, the tip is alone in made and fresh; building on it moves it to
made and consumed, and the input it used to given and consumed.

- In the search tree, a choice is one flat choice among the quadrants holding options (made and fresh first), then one
  flat choice among that quadrant's options.
- In a rollout, a fresh operand is taken over a consumed one with probability `fresh_weight`, then a made one over a
  given one with probability `made_weight`, then any option of the quadrant. With both at 0.5 there is no preference.

## Other seeds (3 minutes each, same settings)

| Seed | This version | The first quadrant version (commit 893dcdf) |
|---|---|---|
| 1 | 4 xors, cost 1046 | cost 19 at 76 s |
| 2 | **cost 19 at 42.7 s** | 1 xor, cost 900 |
| 3 | 4 xors, cost 737 | 8 xors, cost 22 at 86 s |

The first quadrant version had the same tree and rollout odds; it drew its rollout decisions differently. Each found the
full function on one of three seeds. When a run misses, it settles on one shallow root in the first minute or two and
stays there, so this seed is a good draw, not the typical run.

## Reproducing

`swift_parity.cpp` is this run and nothing else: every setting above is a constant in its source, and it takes no
arguments. At the end it checks the result itself: the best model must cost 19, be a leaf with 9 xors, and be right on
all 1024 rows. It prints `PROOF PASSED` and exits with 0, or `PROOF FAILED` with what it found and exits with 1. From
the repository root:

```bash
make research/swift_parity
```

```bash
./build/research/swift_parity
```
