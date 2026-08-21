#!/usr/bin/env python3
"""Benchmark Atlas dbuct-ridge-fc on chc/spine.chc.

Three suites, reported in simulations and seconds to the first solution:

  synth   prog_depth_le(M, Max), fits(M, Ins, Labs)   -- guess a program
  arith   the same, over targets built from plus, mult and pow
  eval    normalize(Term, R)                          -- run a known term

arith takes several minutes: its targets nest two operations, which is
deep enough to be real work. --suite synth is the quick one.

Data is spelled as spines, which is what spine.chc requires: data(emp) for
the empty list, app(app(data(cons), Head), Tail) for a cons, and a number n
as n nested app(data(suc), ...) around data(zero).

Max must fit the POINT-FREE answer. fits applies the candidate to each row's
arguments, so sum is foldr plus 0 at depth two and NOT the lambda that wraps
it -- budgeting for the lambda puts the task out of reach entirely.

Get a row wrong and nothing fits, which looks exactly like a hard task: the
search runs until the timeout and says nothing. Two of the arith tables below
did that while being written. When a task will not solve, check the table by
running fits on the intended answer before blaming the search.

--max-resolutions is a per-simulation budget, and tightening it cuts the cost
of a divergent guess: and-from-if runs 7.7s at 200 against 9.5s at 2000, on
much the same simulation count. It is not free, though. Below what one honest
rollout needs, a solvable task comes back REFUTED rather than slow --
sum-as-foldr does exactly that at 200 and solves at 500. Treat a refutation
under a tight cap as "no program within this budget", never as "no program".

Examples

  ./bench_spine.py                          every suite, seeds 1 2 3
  ./bench_spine.py --suite synth --seeds 1,2,3,4,5
  ./bench_spine.py --suite arith --timeout 90
  ./bench_spine.py --only and-from-if --cap 2000
  ./bench_spine.py --suite synth --bound size   node count instead of depth
  ./bench_spine.py --db /tmp/variant.chc    any file with the same API
  ./bench_spine.py --only sum-as-foldr --show-commands
"""

from __future__ import annotations

import argparse
import os
import queue
import re
import statistics
import subprocess
import sys
import threading
import time
from dataclasses import dataclass

ATLAS_DEFAULT = os.environ.get("ATLAS", "atlas")
DB_DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "spine.chc")

SIM_RE = re.compile(r"^(\d+) sims \|")
MODEL_RE = re.compile(r"^  (?:M|R) = (.*)$")

# ------------------------------------------------------------
# Spine spellings
# ------------------------------------------------------------


def peano(n: int) -> str:
    t = "zero"
    for _ in range(n):
        t = "suc(%s)" % t
    return t


def nat(n: int) -> str:
    t = "data(zero)"
    for _ in range(n):
        t = "app(data(suc), %s)" % t
    return t


def lst(xs: list) -> str:
    t = "data(emp)"
    for x in reversed(xs):
        t = "app(app(data(cons), %s), %s)" % (enc(x), t)
    return t


def enc(v) -> str:
    if isinstance(v, bool):
        return "data(true)" if v else "data(false)"
    if isinstance(v, int):
        return nat(v)
    if isinstance(v, list):
        return lst(v)
    if isinstance(v, str):
        return v
    raise TypeError(v)


def ap(head: str, *args: str) -> str:
    t = head
    for a in args:
        t = "app(%s, %s)" % (t, a)
    return t


# ------------------------------------------------------------
# Suites
# ------------------------------------------------------------


@dataclass
class Synth:
    name: str
    rows: list  # [(inputs, label)]
    max_depth: int  # depth of the POINT-FREE answer
    cap: int  # --max-resolutions
    intended: str


@dataclass
class Eval:
    name: str
    term: str


SYNTH = [
    Synth("not", [([False], True), ([True], False)], 1, 2000, "global(not)"),
    Synth(
        "or",
        [([False, False], False), ([False, True], True),
         ([True, False], True), ([True, True], True)],
        1, 2000, "global(or)",
    ),
    Synth("plus", [([1, 1], 2), ([2, 1], 3), ([0, 3], 3)], 1, 2000, "global(plus)"),
    Synth("if", [([True, 1, 2], 1), ([False, 1, 2], 2)], 1, 2000, "global(if)"),
    Synth(
        "sum-as-foldr",
        [([[]], 0), ([[1, 2]], 3), ([[1, 2, 3]], 6)],
        2, 2000, "app(app(global(foldr), global(plus)), data(zero))",
    ),
    Synth(
        "and-from-if",
        [([False, False], False), ([False, True], False),
         ([True, False], False), ([True, True], True)],
        4, 2000, "abs(app(app(global(if), app(global(not), var(zero))), data(false)))",
    ),
    Synth(
        "map-suc",
        [([[]], []), ([[1]], [2]), ([[1, 2]], [2, 3])],
        2, 2000, "app(global(map), data(suc))",
    ),
]

# Arithmetic, where a target nests one operation inside another. Every
# label is kept small on purpose: a Peano result is built one suc at a
# time, so a big number spends the resolution budget on counting rather
# than on searching. The whole battery verifies inside 900 resolutions.
SQUARE = ap("global(mult)", "var(zero)", "var(zero)")
SUC_N = ap("data(suc)", "var(zero)")
DOUBLE = ap("global(plus)", "var(zero)", "var(zero)")

ARITH = [
    Synth("mult", [([2, 3], 6), ([4, 1], 4), ([0, 5], 0)], 1, 2000, "global(mult)"),
    Synth("pow", [([2, 3], 8), ([3, 2], 9), ([5, 1], 5)], 1, 2000, "global(pow)"),
    Synth("square", [([1], 1), ([2], 4), ([3], 9)], 3, 2000, "abs(%s)" % SQUARE),
    Synth("n-to-the-n", [([1], 1), ([2], 4), ([3], 27)], 3, 2000,
          "abs(%s)" % ap("global(pow)", "var(zero)", "var(zero)")),
    Synth("two-to-the-n", [([0], 1), ([1], 2), ([3], 8)], 3, 2000,
          ap("global(pow)", nat(2))),
    Synth("product-as-foldr", [([[]], 1), ([[2, 3]], 6), ([[2, 3, 4]], 24)], 3, 2000,
          ap("global(foldr)", "global(mult)", nat(1))),
    Synth("double-plus-one", [([0], 1), ([1], 3), ([2], 5)], 3, 2000,
          "abs(%s)" % ap("data(suc)", DOUBLE)),
    Synth("square-plus-one", [([1], 2), ([2], 5), ([3], 10)], 4, 2000,
          "abs(%s)" % ap("data(suc)", SQUARE)),
    Synth("square-plus-n", [([1], 2), ([2], 6), ([3], 12)], 4, 2000,
          "abs(%s)" % ap("global(plus)", SQUARE, "var(zero)")),
    Synth("cube", [([1], 1), ([2], 8), ([3], 27)], 4, 2000,
          "abs(%s)" % ap("global(mult)", "var(zero)", SQUARE)),
    Synth("suc-squared", [([0], 1), ([1], 4), ([2], 9)], 4, 2000,
          "abs(%s)" % ap("global(mult)", SUC_N, SUC_N)),
    Synth("pow-flipped", [([2, 3], 9), ([3, 2], 8), ([4, 1], 1)], 4, 2000,
          "abs(abs(%s))" % ap("global(pow)", "var(zero)", "var(suc(zero))")),
]

EVAL = [
    Eval("plus 20 20", ap("global(plus)", nat(20), nat(20))),
    Eval("map suc [0..15]", ap("global(map)", "data(suc)", lst(list(range(16))))),
    Eval("foldr plus 0 [1..10]",
         ap("global(foldr)", "global(plus)", nat(0), lst(list(range(1, 11))))),
    Eval("map (plus 10) [0..4]",
         ap("global(map)", ap("global(plus)", nat(10)), lst(list(range(5))))),
    Eval("twice twice suc 1", ap("global(twice)", "global(twice)", "data(suc)", nat(1))),
    Eval("lambda n. plus n 3", "abs(%s)" % ap("global(plus)", "var(zero)", nat(3))),
    Eval("cons id emp", ap("data(cons)", "abs(var(zero))", "data(emp)")),
    Eval("mult 6 7", ap("global(mult)", nat(6), nat(7))),
    Eval("pow 2 6", ap("global(pow)", nat(2), nat(6))),
    Eval("map (mult 3) [1..5]",
         ap("global(map)", ap("global(mult)", nat(3)), lst(list(range(1, 6))))),
    Eval("foldr mult 1 [1..5]",
         ap("global(foldr)", "global(mult)", nat(1), lst(list(range(1, 6))))),
]

EVAL_CAP = 2_000_000


def nodes(term: str) -> int:
    """Node count of a term, read off the intended answer.

    Kept derived rather than declared so a task cannot drift out of step
    with the size its own answer needs.
    """
    return sum(term.count(former)
               for former in ("var(", "global(", "data(", "abs(", "app("))


def synth_goal(p: Synth, bound: str) -> str:
    ins = "[" + ", ".join(
        "[" + ", ".join(enc(a) for a in args) + "]" for args, _ in p.rows) + "]"
    labs = "[" + ", ".join(enc(lab) for _, lab in p.rows) + "]"
    if bound == "size":
        limit = "prog_size_le(M, %s)" % peano(nodes(p.intended))
    else:
        limit = "prog_depth_le(M, %s)" % peano(p.max_depth)
    return "%s, fits(M, %s, %s)" % (limit, ins, labs)


def eval_goal(p: Eval) -> str:
    return "normalize(%s, R)" % p.term


# ------------------------------------------------------------
# Runner
# ------------------------------------------------------------


def command(atlas: str, db: str, goal: str, cap: int, seed: int) -> list:
    return [atlas, "dbuct-ridge-fc", db, "-g", goal,
            "--max-resolutions", str(cap), "--seed", str(seed),
            "--grant-increment-interval", "1",
            "--sim-progress-interval", "1"]


def _drain(stream, q):
    for line in stream:
        q.put(line)
    q.put(None)


def run_one(atlas: str, db: str, goal: str, cap: int, seed: int, timeout: float):
    """Seconds and simulations to the FIRST solution.

    The solver prompts for further solutions, so it is killed as soon as the
    model is in hand; letting it run on would time the search for a second
    answer instead.
    """
    try:
        proc = subprocess.Popen(command(atlas, db, goal, cap, seed),
                                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, bufsize=1)
    except OSError as e:
        return "error", str(e), 0.0, 0

    q: queue.Queue = queue.Queue()
    threading.Thread(target=_drain, args=(proc.stdout, q), daemon=True).start()
    start = time.perf_counter()
    status, model, sims = "refuted", "", 0
    try:
        while True:
            left = timeout - (time.perf_counter() - start)
            if left <= 0:
                status = "timeout"
                break
            try:
                line = q.get(timeout=min(left, 0.2))
            except queue.Empty:
                continue
            if line is None:
                break
            m = SIM_RE.match(line)
            if m:
                sims = int(m.group(1))
                continue
            m = MODEL_RE.match(line)
            if m:
                status, model = "solved", m.group(1).strip()
                break
    finally:
        elapsed = time.perf_counter() - start
        proc.kill()
        proc.wait()
    return status, model, elapsed, sims


def report(label: str, rows: list) -> None:
    print("\n%s" % label)
    print("%-22s %-8s %8s %8s  %s" % ("task", "status", "sims", "secs", "model"))
    print("-" * 96)
    for name, status, sims, secs, model in rows:
        print("%-22s %-8s %8s %8s  %s" % (
            name, status, sims if sims else "-", "%.3f" % secs, model[:44]))


def median(xs: list) -> float:
    return statistics.median(xs) if xs else 0.0


def synth_suite(args, tasks: list, seeds: list) -> tuple:
    """Rows to report, plus how many tasks failed and how many were flaky."""
    rows, failures, flaky = [], 0, 0
    for p in tasks:
        goal, cap = synth_goal(p, args.bound), args.cap or p.cap
        runs = [run_one(args.atlas, args.db, goal, cap, s, args.timeout)
                for s in seeds]
        ok = [r for r in runs if r[0] == "solved"]
        if not ok:
            failures += 1
            rows.append((p.name, runs[0][0], 0, median([r[2] for r in runs]),
                         "wanted %s" % p.intended))
            continue
        if len(ok) != len(runs):
            flaky += 1
        rows.append((p.name, "solved" if len(ok) == len(runs) else "flaky",
                     int(median([r[3] for r in ok])),
                     median([r[2] for r in ok]), ok[0][1]))
    return rows, failures, flaky


def main() -> int:
    ap_ = argparse.ArgumentParser()
    ap_.add_argument("--atlas", default=ATLAS_DEFAULT)
    ap_.add_argument("--db", default=DB_DEFAULT)
    ap_.add_argument("--suite", choices=("synth", "arith", "eval", "all"),
                     default="all")
    ap_.add_argument("--bound", choices=("depth", "size"), default="depth",
                     help="bound candidate programs by nesting depth or by node count")
    ap_.add_argument("--seeds", default="1,2,3")
    ap_.add_argument("--timeout", type=float, default=60.0)
    ap_.add_argument("--cap", type=int, default=0, help="override --max-resolutions")
    ap_.add_argument("--only", default="", help="comma separated task names")
    ap_.add_argument("--show-commands", action="store_true",
                     help="print the atlas invocation for each task and exit")
    args = ap_.parse_args()

    seeds = [int(s) for s in args.seeds.split(",") if s.strip()]
    want = {x.strip() for x in args.only.split(",") if x.strip()}
    synth = [p for p in SYNTH if not want or p.name in want]
    ariths = [p for p in ARITH if not want or p.name in want]
    evals = [p for p in EVAL if not want or p.name in want]
    if want:
        missing = (want - {p.name for p in synth} - {p.name for p in ariths}
                   - {p.name for p in evals})
        if missing:
            print("unknown tasks: %s" % sorted(missing), file=sys.stderr)
            return 2

    if args.show_commands:
        shown = []
        if args.suite in ("synth", "all"):
            shown += synth
        if args.suite in ("arith", "all"):
            shown += ariths
        for p in shown:
            print("# %s" % p.name)
            print(" ".join(command(args.atlas, args.db, "'%s'" % synth_goal(p, args.bound),
                                   args.cap or p.cap, seeds[0])) + "\n")
        for p in evals if args.suite in ("eval", "all") else []:
            print("# %s" % p.name)
            print(" ".join(command(args.atlas, args.db, "'%s'" % eval_goal(p),
                                   args.cap or EVAL_CAP, seeds[0])) + "\n")
        return 0

    print("atlas=%s\ndb=%s\nseeds=%s timeout=%ss bound=%s\n"
          "metric: simulations and seconds to the first solution, median over seeds"
          % (args.atlas, args.db, seeds, args.timeout, args.bound))

    failures, flaky = 0, 0
    bound_call = ("prog_size_le(M, Nodes)" if args.bound == "size"
                  else "prog_depth_le(M, Max)")
    if args.suite in ("synth", "all"):
        rows, f, k = synth_suite(args, synth, seeds)
        failures, flaky = failures + f, flaky + k
        report("synthesis: %s, fits(M, Ins, Labs)" % bound_call, rows)

    if args.suite in ("arith", "all"):
        rows, f, k = synth_suite(args, ariths, seeds)
        failures, flaky = failures + f, flaky + k
        report("arithmetic: %s, fits(M, Ins, Labs)" % bound_call, rows)

    if args.suite in ("eval", "all"):
        rows = []
        for p in evals:
            goal, cap = eval_goal(p), args.cap or EVAL_CAP
            runs = [run_one(args.atlas, args.db, goal, cap, s, args.timeout)
                    for s in seeds]
            ok = [r for r in runs if r[0] == "solved"]
            if not ok:
                failures += 1
                rows.append((p.name, runs[0][0], 0, median([r[2] for r in runs]), ""))
                continue
            rows.append((p.name, "solved", int(median([r[3] for r in ok])),
                         min(r[2] for r in ok), ok[0][1]))
        report("evaluation: normalize(Term, R)", rows)

    if failures:
        print("\n%d task(s) did not solve" % failures)
    elif flaky:
        print("\nall solved, %d only on some seeds" % flaky)
    else:
        print("\nall solved on every seed")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
