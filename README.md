# AML — Abstractive Machine Learning

AML is a machine learning system that learns **abstractions** (functions) rather than parameters. Instead of fitting weights in a fixed-topology network, AML searches for the smallest computational graph — the fewest nodes, the most reused functions — that explains the training data. This is a direct implementation of **Occam's Razor as an optimization objective**.

---

## The core idea

Standard machine learning encodes knowledge in billions of floating-point weights distributed across a fixed architecture. AML encodes knowledge in a **program**: a set of named, composable functions that are discovered, refined, and shared across the data.

The key insight is that **abstractive learning is nonlocal**. Whether a helper function earns its place in the model cannot be determined by looking at any one site where it appears — it earns its keep by being useful many times over, across many different parts of the model. This has four immediate consequences:

- **Exponentially smaller models.** AML models are Turing-complete programs — the learned functions can be arbitrarily composed and recursive. This is the root cause of the size reduction: a single well-chosen abstraction can express a computation that a neural network, whose primitives are fixed non-Turing-complete operations (additions, multiplications, activation functions — or in more domain-specific architectures, still bounded, non-learnable-as-programs functions), would need an exponentially large topology to approximate. The model size grows with the complexity of the underlying concept, not with the volume of data.
- **Exponentially less training data.** An abstraction is reused across many different parts of the input within a single data point. This means we squeeze far more information out of each data point we see. A single example provides evidence for a helper function many times over, so far fewer examples are needed to justify it.
- **Strong generalization from small datasets.** Because AML is always searching for Occam's Razor, we can feel confident that a small dataset is enough to produce a model that generalizes well — the simplicity objective acts as the regularizer. 100 data points can be sufficient for surprisingly complex problems when you consider all of the informational structure that can be extracted from them and their intersections. For a simple problem — say, learning a boolean function over four inputs — a handful of data points is enough to find a clean, general model. A neural network given the same data would tend to overfit, memorizing the examples rather than discovering the underlying rule.
- **Faster convergence to correct abstractions.** Because evidence for a helper function arrives from many independent paths through the input simultaneously, the search converges to a correct definition much faster than if each path were evaluated in isolation. This is precisely the failure mode of fully connected neural networks: each node-to-node connection between two layers gets its own dedicated weight, learned from exactly one path of information. There is no sharing, no reuse, and therefore no leverage — every weight must be justified by the data independently.

---

## Why lambda calculus

Occam's Razor, as an optimization objective, comes down to **node count**: the model with the fewest nodes in its computational graph that still explains the data is preferred. This means the choice of computational primitive matters enormously — it determines what counts as one node.

Of the three classical foundations of Turing completeness:

- **Boolean algebra** — primitives are logic gates (AND, OR, NOT). Counting gates works for circuits but there is no native concept of a named, reusable function. Sharing a sub-computation requires duplicating its gates at every call site; abstraction is invisible to the node count.
- **Turing machines** — primitives are tape operations and state transitions. Again, no intrinsic notion of abstraction or substitution. Reuse is not directly expressible in the formalism.
- **Lambda calculus** — primitives are *abstraction* (defining a function) and *application* (calling it). Sharing is first-class: naming a sub-computation and applying it in multiple places is exactly what the formalism is built around, and each named function is a single node regardless of how many times it is used.

Lambda calculus is therefore the natural fit. Its two primitives — abstraction and substitution — map directly onto what Occam's Razor demands: find the smallest set of named functions whose composition explains the data. AML's learned models are lambda-calculus terms, and its node-count objective is measured over those terms.

---

## How AML differs from neural networks

| Property | Neural networks | AML |
|---|---|---|
| Model representation | Fixed-topology weight matrices | Learned program (computational graph) |
| Knowledge encoding | Distributed across weights | Explicit, named functions |
| Model size | Grows with data volume and task complexity | Grows with conceptual complexity only |
| Training data requirement | More intelligence → more data | More intelligence → same data, longer search |
| Improving a trained model | Requires more data or retraining | Run longer; Occam's Razor guides continuous refinement |
| Generalization mechanism | Statistical interpolation | Structural abstraction — exact reuse |
| Interpretability | Black box | The model is a readable program |
| Turing completeness | No (bounded computation per token) | Yes — functions can be arbitrarily composed and recursive |

The last row matters most. Because AML's learned functions are Turing-complete, a single abstraction can capture a computation that would require an arbitrarily deep or wide neural network to approximate. This is where the exponential reduction in model size comes from: **you are not approximating the function, you are finding it**.

---

## The search process

AML frames learning as a **Constrained Horn Clause (CHC) search** over the space of programs, structured as two nested loops:

- **Outer loop** — tracks the smallest model size found so far. The bar starts at infinity — any model that explains the data is acceptable as a first solution. Each time the inner loop succeeds, the bar is set to the size of the model just found.
- **Inner loop** — searches for a model strictly smaller than the current best. Think of it like limbo: the bar starts at the size of whatever model was last found, and the inner loop must get under it. If it succeeds, the bar drops again. If it cannot — if no smaller model exists that still explains the data — the search terminates.

The CHC solver ([atlas](atlas/)) powers the inner loop: it searches the space of candidate programs and either finds one that fits under the bar, or proves that none exists.

Because the bar only ever drops, the process is **anytime**: stopping at any point yields the best model found so far, and running longer only improves it.

### Perfect fit is required

Every candidate model must perfectly explain the training data — 100% accuracy is not a goal, it is a hard constraint. This is how AML avoids a competitive reward structure: accuracy and model size are not traded off against each other. Accuracy is locked at 100% and the only thing that varies is the model architecture.

This is the complete inverse of how neural networks are trained. With a neural network, the architecture is fixed and training varies the weights to maximize accuracy — accuracy is what you optimize, and the architecture is the constraint. In AML, accuracy is the constraint and the architecture is what you optimize.

The practical consequence is that there is no way for the search to "cheat" by finding a smaller model that sacrifices correctness. Every model it considers either explains all of the data or is rejected outright.

---

## Contrastive learning

AML learns from both positive and negative data. But it does not work by predicting an output label from an input. The distinction is fundamental to how the model is structured.

Consider learning multiplication. A conventional model would learn a function `f(x, y) → z` — it takes inputs and produces an output. AML instead learns a **relation**: `x * y = z`. The model does not have a designated output; it describes a constraint that holds over all of its arguments together.

This means inference works differently. Rather than feeding inputs into a model and reading off an output, you populate *some* of the variables — whichever ones you know — and run the CHC solver to instantiate the rest. If you know `x` and `y`, the solver finds `z`. If you know `y` and `z`, the solver finds `x`. The same model supports all of these query directions without retraining.

Positive examples are data points where the relation holds. Negative examples are data points where it does not. The search finds the smallest program — the simplest relation — that is consistent with all positive examples and inconsistent with all negative examples.

### Decoupling learnability from inference complexity

This approach has a profound benefit: it severs **predictive accuracy** from **computational complexity of inference**. With a conventional predictive model, these two things are entangled in a way that causes serious problems.

Consider asking a neural network to predict the prime factors of a semiprime — a product of two large primes. The network must not only learn the *relationship* between the product and its factors, it must also be computationally large enough to actually *perform* the factorization internally. If the network is not sophisticated enough to carry out that computation, it will fail to learn anything at all — not even a partial understanding of the relationship. Learnability and computational power are coupled, and if the model falls short on one it fails on both.

With contrastive learning the picture is completely different. AML learns the relation `x = y * z`. This relation is extremely simple to describe — it is just multiplication stated without a fixed direction of information flow. The model captures the relationship between the three quantities without needing to know which ones will be given and which will be solved for. The hard computational work of actually finding the factors is then delegated to the CHC solver at inference time.

The relation of factorization is easy to *learn* even though factorization is hard to *compute*. These two things are no longer the same problem. This further reduces model size and makes a much larger class of problems tractable to learn from data.

A further benefit is correctness. When inference is delegated to the CHC solver, the solver either finds an answer that satisfies the learned relation exactly or it does not — there is no approximation, no hallucination, no fuzzy output. A conventional predictive model, by contrast, is inherently fuzzy: it produces a best guess, and that guess can be wrong in ways that are difficult to detect or bound. By keeping the model as a precise logical relation and letting the solver do the computation, AML guarantees that every inference it produces is consistent with what was learned.

### Personal note

I believe the entanglement of learnability and inference complexity is one of the major reasons why protein folding was so hard to learn from data for so long. Simulating the physical process of a protein folding — computing the actual trajectory from amino acid sequence to final structure — is extraordinarily computationally demanding. A predictive model that must perform that simulation internally in order to produce a correct output needs to be correspondingly large and complex, and learning such a model from data is extremely difficult.

But *describing* the constraints of protein folding — the relation between sequence and structure — may be far simpler than simulating the process. A contrastive learner would not need to simulate anything. It would learn the relation, and then delegate the job of finding structures consistent with that relation to the solver. Whether this framing would have made the problem more tractable earlier is an open question, but the principle seems directly applicable.

---

## Building

**Prerequisites**

- `g++` with C++20 support
- `make`
- Git (for submodule initialization)

**Initialize submodules**

```bash
git submodule update --init --recursive
```

**Build targets**

```bash
make core_debug        # atlas core lib + aml core lib + core test binary
make core_debug_fast   # same with -O3
make cli_debug         # + cli lib + cli test binary
make aml               # release binary  →  build/aml
make all               # everything
make clean
```

**Run tests**

```bash
./build/core_debug                                    # all core tests
./build/core_debug --gtest_filter='MyComponent.*'    # filtered
./build/cli_debug                                     # all cli tests
```

---

## Dependencies

| Submodule | Purpose | URL |
|---|---|---|
| `atlas` | CHC solver — search engine for AML | `git@github.com:team-affix/atlas.git` |
| `CLI11` | Command-line argument parsing | `git@github.com:team-affix/CLI11.git` |
| `googletest` | Unit and integration test framework | `https://github.com/google/googletest.git` |
