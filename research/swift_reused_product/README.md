# Swift on a reused product

Swift finds f(x, y, z) = (x · y + z)² − x · y from 64 of its values, as the formula itself: one leaf outputting
`sub(square(add(z, mul(x, y))), mul(x, y))`, cost 10. The product x · y appears twice, so the formula needs a feature
reused after it has been built on. It is right on 200 unseen inputs drawn up to 30, ten times the training range.

## Setup

- Numbers are binary naturals, a user type with the constructors zero, twice(n) (2n) and twice_plus_one(n) (2n + 1).
- Data: every (x, y, z) with each input from 0 to 3 (64 rows); the label is (x · y + z)² − x · y, up to 135.
- Primitives, nine, each result capped at 2⁴⁰: add, sub (truncated at zero), mul, square, succ, pred (truncated at
  zero), double, half, and muladd (a · b + c). Constants: 0 and 1. `max_applications` = 9.
- Solver: Swift (`swift_runtime`), c = 4, w = 1, `fresh_weight` 0.75, `made_weight` 0.5, seed 1, 600,000 episodes.
- Unseen data: 200 other (x, y, z), each input drawn from 0 to 30 with a fixed seed.

## Result

`build/research/swift_reused_product`, about 4,200 episodes per second:

| Episode | Best cost | Best model's root |
|---|---|---|
| about 150,000 | 232 | `match mul(x, y)` |
| about 300,000 to 480,000 | 230 to 213 | `match muladd(x, y, 0)` |
| **574,584** | **10** | **`leaf sub(square(add(z, mul(x, y))), mul(x, y))`** |

The best model is right on all 64 training rows and all 200 unseen rows. `proof_check.log` is this program's run, with
the same new best models at the same episodes as the experiment it was cut down from.

## What it took

With the four primitives add, sub, mul and square alone, Swift did not find this formula in 3-minute runs at c = 2 or
5, nor with `fresh_weight` 0.5, nor with 200 rows of wider data: each run settled on matching x · y and memorizing the
rows bit by bit. This run added five primitives (succ, pred, double, half, muladd) and used c = 4; the formula it found
uses none of them.

## Reproducing

`swift_reused_product.cpp` is this run and nothing else: every setting is a constant in its source (the seed is 1),
its primitives are defined in it, and it takes no arguments, so every run makes the same episodes. At the end it checks
the result itself: the best model must cost 10, be exactly the leaf `sub(square(add(z, mul(x, y))), mul(x, y))`, and
be right on every training row and every unseen row. It prints `PROOF PASSED` and exits with 0, or `PROOF FAILED`
with what it found and exits with 1. From the repository root:

```bash
make research/swift_reused_product
```

```bash
./build/research/swift_reused_product
```
