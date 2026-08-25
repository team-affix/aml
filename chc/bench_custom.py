#!/usr/bin/env python3
"""Benchmark Atlas dbuct-genius-fc on the custom.chc kernel.

Same task tables as bench_spine.py, translated into custom's term
language: t(name) for named functions, o(C) for constructors, lam
for abstraction, Peano nats as nested suc, lists as nil/cons.

The database is the concatenation of custom.chc, custom_synth.chc and
custom_delta.chc. Synthesis goals are:

  nf(M, zero), size(M, S), le(S, Max), fits(M, Ins, Labs)     --bound size
  nf(M, zero), depth(M, D), le(D, Max), fits(M, Ins, Labs)    --bound depth

--mode test concatenates custom.chc, custom_delta.chc and custom_test.chc
and runs every named test_* predicate. Positive tests must SOLVE;
conflict tests (those under conflict_main) must REFUTE.

Peano arithmetic is linear in the magnitude, so the eval suite uses
smaller numbers than spine's binary nats. Synthesis example tables are
the same as spine's; a tight --max-resolutions cap may refute an honest
answer if evaluating it already exceeds the budget.

Examples

  ./bench_custom.py
  ./bench_custom.py --suite synth --seeds 1,2,3
  ./bench_custom.py --mode test
  ./bench_custom.py --mode test --only test_norm_map_suc
  ./bench_custom.py --only plus,mult --cap 2000
  ./bench_custom.py --only map-suc --show-commands

Peano arithmetic is linear in the magnitude, so the eval suite uses
smaller numbers than spine's binary nats. Synthesis example tables are
the same as spine's; a tight --max-resolutions cap may refute an honest
answer if evaluating it already exceeds the budget.

Examples

  ./bench_custom.py
  ./bench_custom.py --suite synth --seeds 1,2,3
  ./bench_custom.py --suite arith --bound size --timeout 90
  ./bench_custom.py --only plus,mult --cap 2000
  ./bench_custom.py --only map-suc --show-commands
"""

from __future__ import annotations

import argparse
import os
import queue
import re
import statistics
import subprocess
import sys
import tempfile
import threading
import time
from dataclasses import dataclass

ATLAS_DEFAULT = os.environ.get("ATLAS", "atlas")
HERE = os.path.dirname(os.path.abspath(__file__))
BENCH_SOURCES = [
    os.path.join(HERE, "custom.chc"),
    os.path.join(HERE, "custom_synth.chc"),
    os.path.join(HERE, "custom_delta.chc"),
]
TEST_SOURCES = [
    os.path.join(HERE, "custom.chc"),
    os.path.join(HERE, "custom_delta.chc"),
    os.path.join(HERE, "custom_test.chc"),
]
TEST_FILE = os.path.join(HERE, "custom_test.chc")

SIM_RE = re.compile(r"^(\d+) sims \|")
MODEL_RE = re.compile(r"^  (?:M|R) = (.*)$")
SOLVED_RE = re.compile(r"^SOLVED")
TEST_HEAD_RE = re.compile(r"^(test_\w+)\s*:-")
CONFLICT_HEAD_RE = re.compile(r"^conflict_main\s*:-\s*(test_\w+)\s*\.")

FORMERS = ("var(", "o(", "t(", "lam(", "app(", "switch(", "fresh", "fail")


# ------------------------------------------------------------
# Custom spellings
# ------------------------------------------------------------


def peano(n: int) -> str:
    t = "zero"
    for _ in range(n):
        t = "suc(%s)" % t
    return t


def nat(n: int) -> str:
    if n < 0:
        raise ValueError(n)
    t = "o(zero)"
    for _ in range(n):
        t = "app(o(suc), %s)" % t
    return t


def lst(xs: list) -> str:
    t = "o(nil)"
    for x in reversed(xs):
        t = "app(app(o(cons), %s), %s)" % (enc(x), t)
    return t


def enc(v) -> str:
    if isinstance(v, bool):
        return "o(true)" if v else "o(false)"
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


def nodes(term: str) -> int:
    return sum(term.count(former) for former in FORMERS)


def concat_db(sources: list, name: str) -> str:
    path = os.path.join(tempfile.gettempdir(), name)
    parts = []
    for src in sources:
        with open(src) as f:
            parts.append(f.read())
    with open(path, "w") as out:
        out.write("\n\n".join(parts))
        if not parts[-1].endswith("\n"):
            out.write("\n")
    return path


def load_unit_tests() -> tuple:
    """Positive test_* names and conflict test_* names from custom_test.chc."""
    positive, conflict = [], []
    with open(TEST_FILE) as f:
        for line in f:
            m = CONFLICT_HEAD_RE.match(line)
            if m:
                conflict.append(m.group(1))
                continue
            m = TEST_HEAD_RE.match(line)
            if m:
                positive.append(m.group(1))
    conflict_set = set(conflict)
    positive = [n for n in positive if n not in conflict_set]
    return positive, conflict


# ------------------------------------------------------------
# Suites — same problems as bench_spine.py
# ------------------------------------------------------------


@dataclass
class Synth:
    name: str
    rows: list
    max_depth: int
    max_nodes: int
    cap: int
    intended: str


@dataclass
class Eval:
    name: str
    term: str


SYNTH = [
    Synth("not", [([False], True), ([True], False)], 0, 1, 2000, "t(not)"),
    Synth(
        "or",
        [([False, False], False), ([False, True], True),
         ([True, False], True), ([True, True], True)],
        0, 1, 2000, "t(or)",
    ),
    Synth("plus", [([1, 1], 2), ([2, 1], 3), ([0, 3], 3)], 0, 1, 2000, "t(plus)"),
    Synth("if", [([True, 1, 2], 1), ([False, 1, 2], 2)], 0, 1, 2000, "t(if)"),
    Synth(
        "sum-as-foldr",
        [([[]], 0), ([[1, 2]], 3), ([[1, 2, 3]], 6)],
        2, 5, 2000, "app(app(t(foldr), t(plus)), o(zero))",
    ),
    Synth(
        "and-from-if",
        [([False, False], False), ([False, True], False),
         ([True, False], False), ([True, True], True)],
        4, 8, 2000,
        "lam(app(app(t(if), app(t(not), var(zero))), o(false)))",
    ),
    Synth(
        "map-suc",
        [([[]], []), ([[1]], [2]), ([[1, 2]], [2, 3])],
        1, 3, 2000, "app(t(map), o(suc))",
    ),
]

SQUARE = ap("t(mult)", "var(zero)", "var(zero)")
SUC_N = ap("o(suc)", "var(zero)")

ARITH = [
    Synth("mult", [([2, 3], 6), ([4, 1], 4), ([0, 5], 0)], 0, 1, 2000, "t(mult)"),
    Synth("pow", [([2, 3], 8), ([3, 2], 9), ([5, 1], 5)], 0, 1, 2000, "t(pow)"),
    Synth("square", [([1], 1), ([2], 4), ([3], 9)], 3, 6, 2000, "lam(%s)" % SQUARE),
    Synth("n-to-the-n", [([1], 1), ([2], 4), ([3], 27)], 3, 6, 2000,
          "lam(%s)" % ap("t(pow)", "var(zero)", "var(zero)")),
    Synth("two-to-the-n", [([0], 1), ([1], 2), ([3], 8)], 3, 7, 2000,
          ap("t(pow)", nat(2))),
    Synth("product-as-foldr", [([[]], 1), ([[2, 3]], 6), ([[2, 3, 4]], 24)], 2, 7, 2000,
          ap("t(foldr)", "t(mult)", nat(1))),
    Synth("double-plus-one", [([0], 1), ([1], 3), ([2], 5)], 3, 8, 2000,
          "lam(%s)" % ap("t(plus)", "var(zero)", SUC_N)),
    Synth("square-plus-one", [([1], 2), ([2], 5), ([3], 10)], 4, 8, 2000,
          "lam(%s)" % ap("o(suc)", SQUARE)),
    Synth("square-plus-n", [([1], 2), ([2], 6), ([3], 12)], 4, 8, 2000,
          "lam(%s)" % ap("t(mult)", SUC_N, "var(zero)")),
    Synth("cube", [([1], 1), ([2], 8), ([3], 27)], 4, 8, 2000,
          "lam(%s)" % ap("t(twice)", ap("t(mult)", "var(zero)"), "var(zero)")),
    Synth("suc-squared", [([0], 1), ([1], 4), ([2], 9)], 4, 10, 2000,
          "lam(%s)" % ap("t(mult)", SUC_N, SUC_N)),
    Synth("pow-flipped", [([2, 3], 9), ([3, 2], 8), ([4, 1], 1)], 4, 7, 2000,
          "lam(lam(%s))" % ap("t(pow)", "var(zero)", "var(suc(zero))")),
]

X = "var(suc(zero))"
Y = "var(zero)"
N = "var(zero)"

COMPLEX = [
    Synth("times-three", [([0], 0), ([1], 3), ([2], 6)], 4, nodes(ap("t(mult)", nat(3))),
          2000, ap("t(mult)", nat(3))),
    Synth("n-times-n-plus-2", [([0], 0), ([1], 3), ([2], 8)], 4, 10, 2000,
          "lam(%s)" % ap("t(mult)", N, ap("o(suc)", ap("o(suc)", N)))),
    Synth("n-to-the-suc-n", [([1], 1), ([2], 8), ([3], 81)], 3, 8, 2000,
          "lam(%s)" % ap("t(pow)", N, ap("o(suc)", N))),
    Synth("suc-n-to-the-n", [([1], 2), ([2], 9), ([3], 64)], 4, 8, 2000,
          "lam(%s)" % ap("t(pow)", ap("o(suc)", N), N)),
    Synth("map-double", [([[1]], [2]), ([[1, 2]], [2, 4])], 4, 8, 2000,
          ap("t(map)", "lam(%s)" % ap("t(plus)", N, N))),
    Synth("two-n-squared", [([1], 2), ([2], 8)], 4, 12, 2000,
          "lam(%s)" % ap("t(twice)", ap("t(mult)", N), nat(2))),
    Synth("x-times-suc-y", [([2, 1], 4), ([3, 1], 6), ([3, 2], 9)], 4, 9, 2000,
          "lam(lam(%s))" % ap("t(mult)", X, ap("o(suc)", Y))),
    Synth("suc-x-to-the-y", [([1, 2], 4), ([2, 2], 9), ([1, 3], 8)], 3, 6, 2000,
          "lam(%s)" % ap("t(pow)", ap("o(suc)", N))),
    Synth("sum-of-sucs", [([[1]], 2), ([[1, 2]], 5)], 4, 12, 2000,
          "lam(%s)" % ap(ap("t(foldr)", "t(plus)", nat(0)),
                         ap("t(map)", "o(suc)", N))),
]

# Eval numbers are smaller than spine's: Peano plus/mult/pow scale with
# the magnitude, not with bit width.
EVAL = [
    Eval("plus 4 4", ap("t(plus)", nat(4), nat(4))),
    Eval("map suc [0..4]", ap("t(map)", "o(suc)", lst(list(range(5))))),
    Eval("foldr plus 0 [1..4]",
         ap("t(foldr)", "t(plus)", nat(0), lst(list(range(1, 5))))),
    Eval("map (plus 2) [0..3]",
         ap("t(map)", ap("t(plus)", nat(2)), lst(list(range(4))))),
    Eval("twice twice suc 1", ap("t(twice)", "t(twice)", "o(suc)", nat(1))),
    Eval("lambda n. plus n 2", "lam(%s)" % ap("t(plus)", "var(zero)", nat(2))),
    Eval("cons id nil", ap("o(cons)", "lam(var(zero))", "o(nil)")),
    Eval("mult 3 3", ap("t(mult)", nat(3), nat(3))),
    Eval("pow 2 3", ap("t(pow)", nat(2), nat(3))),
    Eval("map (mult 2) [1..3]",
         ap("t(map)", ap("t(mult)", nat(2)), lst(list(range(1, 4))))),
    Eval("foldr mult 1 [1..3]",
         ap("t(foldr)", "t(mult)", nat(1), lst(list(range(1, 4))))),
]

EVAL_CAP = 2_000_000


def synth_goal(p: Synth, bound: str) -> str:
    ins = "[" + ", ".join(
        "[" + ", ".join(enc(a) for a in args) + "]" for args, _ in p.rows) + "]"
    labs = "[" + ", ".join(enc(lab) for _, lab in p.rows) + "]"
    if bound == "size":
        limit = "nf(M, zero), size(M, S), le(S, %s)" % peano(p.max_nodes)
    else:
        limit = "nf(M, zero), depth(M, D), le(D, %s)" % peano(p.max_depth)
    return "%s, fits(M, %s, %s)" % (limit, ins, labs)


def eval_goal(p: Eval) -> str:
    return "normalize(%s, R)" % p.term


# ------------------------------------------------------------
# Runner
# ------------------------------------------------------------


def command(atlas: str, db: str, goal: str, cap: int, seed: int,
            solver: str = "dbuct-genius-fc") -> list:
    return [atlas, solver, db, "-g", goal,
            "--max-resolutions", str(cap), "--seed", str(seed),
            "--grant-increment-interval", "1",
            "--sim-progress-interval", "100"] if solver != "basic" else [
        atlas, "basic", db, "-g", goal,
        "--max-resolutions", str(cap), "--seed", str(seed),
        "--sim-progress-interval", "100"]


def _drain(stream, q):
    for line in stream:
        q.put(line)
    q.put(None)


def run_one(atlas: str, db: str, goal: str, cap: int, seed: int, timeout: float,
            solver: str = "dbuct-genius-fc"):
    """Seconds and simulations to the FIRST solution.

    The solver prompts for further solutions, so it is killed as soon as the
    model is in hand; letting it run on would time the search for a second
    answer instead. Ground goals with no bindings count as solved on SOLVED.
    """
    try:
        proc = subprocess.Popen(command(atlas, db, goal, cap, seed, solver),
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
            if SOLVED_RE.match(line):
                status = "solved"
                continue
            m = MODEL_RE.match(line)
            if m:
                status, model = "solved", m.group(1).strip()
                break
            if status == "solved" and line.startswith("[press Enter"):
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


def run_unit_tests(args, positive: list, conflict: list) -> int:
    cap = args.cap or 2000
    print("atlas=%s\ndb=%s\ntimeout=%ss cap=%s\nunit tests from custom_test.chc"
          % (args.atlas, args.db, args.timeout, cap), flush=True)
    print("\n%-28s %-8s %8s %8s  %s" % ("test", "status", "sims", "secs", "expect"),
          flush=True)
    print("-" * 80, flush=True)
    failures = 0
    cases = ([(n, "solved") for n in positive]
             + [(n, "refuted") for n in conflict])
    for name, expect in cases:
        status, model, secs, sims = run_one(
            args.atlas, args.db, name, cap, 1, args.timeout, solver="basic")
        ok = status == expect
        if not ok:
            failures += 1
        shown = status if ok else "%s (wanted %s)" % (status, expect)
        print("%-28s %-8s %8s %8s  %s" % (
            name, "pass" if ok else "FAIL",
            sims if sims else "-", "%.3f" % secs, shown),
              flush=True)
    if failures:
        print("\n%d test(s) failed" % failures)
    else:
        print("\nall %d tests passed" % len(cases))
    return 1 if failures else 0


def synth_suite(args, tasks: list, seeds: list) -> tuple:
    rows, failures, flaky = [], 0, 0
    for p in tasks:
        goal, cap = synth_goal(p, args.bound), args.cap or p.cap
        runs = [run_one(args.atlas, args.db, goal, cap, s, args.timeout)
                for s in seeds]
        ok = [r for r in runs if r[0] == "solved"]
        if not ok:
            failures += 1
            row = (p.name, runs[0][0], 0, median([r[2] for r in runs]),
                   "wanted %s" % p.intended)
        else:
            if len(ok) != len(runs):
                flaky += 1
            row = (p.name, "solved" if len(ok) == len(runs) else "flaky",
                   int(median([r[3] for r in ok])),
                   median([r[2] for r in ok]), ok[0][1])
        rows.append(row)
        print("%-22s %-8s %8s %8s  %s" % (
            row[0], row[1], row[2] if row[2] else "-", "%.3f" % row[3], row[4][:44]),
              flush=True)
    return rows, failures, flaky


def main() -> int:
    ap_ = argparse.ArgumentParser()
    ap_.add_argument("--mode", choices=("bench", "test"), default="bench",
                     help="bench: synthesis/eval suites; test: custom_test.chc unit tests")
    ap_.add_argument("--atlas", default=ATLAS_DEFAULT)
    ap_.add_argument("--db", default="",
                     help="CHC file (default: concat of the files that mode needs)")
    ap_.add_argument("--suite", choices=("synth", "arith", "complex", "eval", "all"),
                     default="all")
    ap_.add_argument("--bound", choices=("depth", "size"), default="size",
                     help="bound candidate programs by nesting depth or by node count")
    ap_.add_argument("--seeds", default="1,2,3")
    ap_.add_argument("--timeout", type=float, default=60.0)
    ap_.add_argument("--cap", type=int, default=0, help="override --max-resolutions")
    ap_.add_argument("--only", default="", help="comma separated task names")
    ap_.add_argument("--show-commands", action="store_true",
                     help="print the atlas invocation for each task and exit")
    args = ap_.parse_args()

    if args.db:
        args.db = os.path.abspath(args.db)
    elif args.mode == "test":
        args.db = concat_db(TEST_SOURCES, "custom_test_all.chc")
    else:
        args.db = concat_db(BENCH_SOURCES, "custom_bench.chc")

    want = {x.strip() for x in args.only.split(",") if x.strip()}

    if args.mode == "test":
        positive, conflict = load_unit_tests()
        if want:
            def matches(n):
                return n in want or n.removeprefix("test_") in want
            positive = [n for n in positive if matches(n)]
            conflict = [n for n in conflict if matches(n)]
            known = set(positive) | set(conflict)
            missing = want - known - {n.removeprefix("test_") for n in known}
            if missing and not positive and not conflict:
                print("unknown tests: %s" % sorted(missing), file=sys.stderr)
                return 2
        if args.show_commands:
            cap = args.cap or 2000
            for n in positive + conflict:
                print("# %s" % n)
                print(" ".join(command(args.atlas, args.db, n, cap, 1, "basic")) + "\n")
            return 0
        return run_unit_tests(args, positive, conflict)

    seeds = [int(s) for s in args.seeds.split(",") if s.strip()]
    synth = [p for p in SYNTH if not want or p.name in want]
    ariths = [p for p in ARITH if not want or p.name in want]
    complexs = [p for p in COMPLEX if not want or p.name in want]
    evals = [p for p in EVAL if not want or p.name in want]
    if want:
        missing = (want - {p.name for p in synth} - {p.name for p in ariths}
                   - {p.name for p in complexs} - {p.name for p in evals})
        if missing:
            print("unknown tasks: %s" % sorted(missing), file=sys.stderr)
            return 2

    if args.show_commands:
        shown = []
        if args.suite in ("synth", "all"):
            shown += synth
        if args.suite in ("arith", "all"):
            shown += ariths
        if args.suite == "complex":
            shown += complexs
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
    bound_call = ("nf(M, zero), size(M, S), le(S, Nodes)" if args.bound == "size"
                  else "nf(M, zero), depth(M, D), le(D, Max)")
    if args.suite in ("synth", "all"):
        print("\nsynthesis: %s, fits(M, Ins, Labs)" % bound_call, flush=True)
        print("%-22s %-8s %8s %8s  %s" % ("task", "status", "sims", "secs", "model"),
              flush=True)
        print("-" * 96, flush=True)
        rows, f, k = synth_suite(args, synth, seeds)
        failures, flaky = failures + f, flaky + k

    if args.suite in ("arith", "all"):
        print("\narithmetic: %s, fits(M, Ins, Labs)" % bound_call, flush=True)
        print("%-22s %-8s %8s %8s  %s" % ("task", "status", "sims", "secs", "model"),
              flush=True)
        print("-" * 96, flush=True)
        rows, f, k = synth_suite(args, ariths, seeds)
        failures, flaky = failures + f, flaky + k

    if args.suite == "complex":
        print("\ncomplex: %s, fits(M, Ins, Labs)" % bound_call, flush=True)
        print("%-22s %-8s %8s %8s  %s" % ("task", "status", "sims", "secs", "model"),
              flush=True)
        print("-" * 96, flush=True)
        rows, f, k = synth_suite(args, complexs, seeds)
        failures, flaky = failures + f, flaky + k

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
