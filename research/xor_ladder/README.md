# The xor ladder

Does each nested xor at the root of a 10-bit parity model improve the reward that Razor's rollouts see?

## Setup

- Data: the full truth table of 10 bits (1024 rows); the inputs x0..x9 are the bits, and the label is their parity.
- Primitives: xor (id 0), and, or, not. Constants: false, true. `max_applications` = 9.
- The root is fixed to match on the nested xor of x0..xk: `x0` for k = 0, `xor(x0, x1)` for k = 1,
  `xor(xor(x0, x1), x2)` for k = 2, and so on up to the full parity at k = 9.
- Everything below the root is built exactly as a Razor rollout builds it: every choice (stop or continue, primitive,
  arguments, scrutinee, bound argument) is uniformly random.
- 2000 samples per root; cost is the model's size (lower is better, and the reward is its negation).

## Results

Seed 1, `xor_ladder 9 1 2000`:

| Xors in the root | Root feature size | Mean cost | Change from one fewer xor | sd | se | Best sample |
|---|---|---|---|---|---|---|
| 0 | 1 | 2234.3 | | 31.2 | 0.7 | 2056 |
| 1 | 3 | 2127.5 | −106.8 | 66.9 | 1.5 | 1889 |
| 2 | 5 | 2025.8 | −101.7 | 84.0 | 1.9 | 1698 |
| 3 | 7 | 1880.6 | −145.2 | 113.9 | 2.5 | 1515 |
| 4 | 9 | 1696.5 | −184.1 | 147.5 | 3.3 | 1153 |
| 5 | 11 | 1435.2 | −261.3 | 185.7 | 4.2 | 712 |
| 6 | 13 | 1131.9 | −303.3 | 211.2 | 4.7 | 157 |
| 7 | 15 | 766.1 | −365.8 | 221.2 | 4.9 | 28 |
| 8 | 17 | 343.8 | −422.3 | 191.2 | 4.3 | 21 |
| 9 | 19 | 22.0 | −321.8 | 0.0 | 0.0 | 22 |

Every added xor lowers the mean rollout cost, by 50 to 100 standard errors, and the gain grows as the xor deepens. The
reward therefore favours each next step towards parity, even when everything below the root is random.

## Reproducing

From the repository root:

```bash
make core
```

```bash
g++ -std=c++20 -O3 -I. -Icore/hpp -Imcts/core/hpp -Icore/test research/xor_ladder/xor_ladder.cpp -Lbuild -laml_core -o build/xor_ladder
```

```bash
build/xor_ladder 9 1 2000
```
