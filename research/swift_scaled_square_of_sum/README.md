# Swift on the scaled square of a sum

Swift finds f(x, y, z) = (x + y)² · z from 64 of its values, as the formula itself: one leaf outputting
`mul(square(add(x, y)), z)`, cost 6. It is right on every unseen input tried, so it is the function, not a table of
the training rows.

## Setup

- Numbers are binary naturals, a user type with the constructors zero, twice(n) (2n) and twice_plus_one(n) (2n + 1),
  so a number takes about log₂ of its value in nodes. (Peano naturals, one successor per unit, grow too large: random
  rollouts square and multiply numbers into the millions.)
- Data: every (x, y, z) with each input from 0 to 3 (64 rows); the label is (x + y)² · z, up to 108.
- Primitives: add, sub (truncated at zero), mul and square, each capped at 2⁴⁰. Constants: 0 and 1.
  `max_applications` = 9.
- Solver: Swift (`swift_runtime`) with the settings of the parity proof but seed 1: c = 2, w = 1, `fresh_weight` 0.75,
  `made_weight` 0.5, 10,000 episodes.
- Unseen data: every other (x, y, z) with each input from 0 to 5 (152 rows).

## Result

`build/research/swift_scaled_square_of_sum`, about 4,500 episodes per second:

| Episode | Best cost | Best model's root |
|---|---|---|
| 165 | 262 | `match add(x, y)` |
| 4,726 | 9 | `leaf mul(mul(add(x, y), add(x, y)), z)` |
| **9,382** | **6** | **`leaf mul(square(add(x, y)), z)`** |

At episode 4,726 (1 second here) Swift has the function with the square written as a product; at episode 9,382 (2.1
seconds) it has the smallest form these primitives allow. The best model is right on all 64 training rows and all 152
unseen rows. `proof_check.log` is this program's run, with the same new best models at the same episodes as the
experiment it was cut down from.

## Reproducing

`swift_scaled_square_of_sum.cpp` is this run and nothing else: every setting is a constant in its source (the
seed is 1), its primitives are defined in it, and it takes no arguments, so every run makes the same episodes. At the
end it checks the result itself: the best model must cost 6, be exactly the leaf `mul(square(add(x, y)), z)`, and be
right on every training and unseen row. It prints `PROOF PASSED` and exits with 0, or `PROOF FAILED` with what it
found and exits with 1. From the repository root:

```bash
make research/swift_scaled_square_of_sum
```

```bash
./build/research/swift_scaled_square_of_sum
```
