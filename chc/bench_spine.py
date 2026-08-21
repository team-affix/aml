#!/usr/bin/env python3
"""Benchmark Atlas dbuct-ridge-fc on chc/spine.chc.

Two suites, reported in simulations and seconds to the first solution:

  synth   prog_depth_le(M, Max), fits(M, Ins, Labs)   -- guess a program
  eval    normalize(Term, R)                          -- run a known term

Data is spelled as spines, which is what spine.chc requires: data(emp) for
the empty list, app(app(data(cons), Head), Tail) for a cons, and a number n
as n nested app(data(suc), ...) around data(zero).

Max must fit the POINT-FREE answer. fits applies the candidate to each row's
arguments, so sum is foldr plus 0 at depth two and NOT the lambda that wraps
it -- budgeting for the lambda puts the task out of reach entirely.

--max-resolutions is a per-simulation budget, and tightening it cuts the cost
of a divergent guess: and-from-if runs 7.7s at 200 against 9.5s at 2000, on
much the same simulation count. It is not free, though. Below what one honest
rollout needs, a solvable task comes back REFUTED rather than slow --
sum-as-foldr does exactly that at 200 and solves at 500. Treat a refutation
under a tight cap as "no program within this budget", never as "no program".

Examples

  ./bench_spine.py                          both suites, seeds 1 2 3
  ./bench_spine.py --suite synth --seeds 1,2,3,4,5
  ./bench_spine.py --only and-from-if --cap 2000
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
]

EVAL_CAP = 2_000_000


def synth_goal(p: Synth) -> str:
    ins = "[" + ", ".join(
        "[" + ", ".join(enc(a) for a in args) + "]" for args, _ in p.rows) + "]"
    labs = "[" + ", ".join(enc(lab) for _, lab in p.rows) + "]"
    return "prog_depth_le(M, %s), fits(M, %s, %s)" % (peano(p.max_depth), ins, labs)


def eval_goal(p: Eval) -> str:
    return "normalize(%s, R)" % p.term


# ------------------------------------------------------------
# Runner
# ------------------------------------------------------------


def command(atlas: str, db: str, goal: str, cap: int, seed: int) -> list:
    return [atlas, "dbuct-ridge-fc", db, "-g", goal,
            "--max-resolutions", str(cap), "--seed", str(seed),
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


def main() -> int:
    ap_ = argparse.ArgumentParser()
    ap_.add_argument("--atlas", default=ATLAS_DEFAULT)
    ap_.add_argument("--db", default=DB_DEFAULT)
    ap_.add_argument("--suite", choices=("synth", "eval", "all"), default="all")
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
    evals = [p for p in EVAL if not want or p.name in want]
    if want:
        missing = want - {p.name for p in synth} - {p.name for p in evals}
        if missing:
            print("unknown tasks: %s" % sorted(missing), file=sys.stderr)
            return 2

    if args.show_commands:
        for p in synth if args.suite in ("synth", "all") else []:
            print("# %s" % p.name)
            print(" ".join(command(args.atlas, args.db, "'%s'" % synth_goal(p),
                                   args.cap or p.cap, seeds[0])) + "\n")
        for p in evals if args.suite in ("eval", "all") else []:
            print("# %s" % p.name)
            print(" ".join(command(args.atlas, args.db, "'%s'" % eval_goal(p),
                                   args.cap or EVAL_CAP, seeds[0])) + "\n")
        return 0

    print("atlas=%s\ndb=%s\nseeds=%s timeout=%ss\n"
          "metric: simulations and seconds to the first solution, median over seeds"
          % (args.atlas, args.db, seeds, args.timeout))

    failures = 0
    if args.suite in ("synth", "all"):
        rows = []
        for p in synth:
            goal, cap = synth_goal(p), args.cap or p.cap
            runs = [run_one(args.atlas, args.db, goal, cap, s, args.timeout)
                    for s in seeds]
            ok = [r for r in runs if r[0] == "solved"]
            if not ok:
                failures += 1
                rows.append((p.name, runs[0][0], 0, median([r[2] for r in runs]),
                             "wanted %s" % p.intended))
                continue
            rows.append((p.name, "solved" if len(ok) == len(runs) else "flaky",
                         int(median([r[3] for r in ok])),
                         median([r[2] for r in ok]), ok[0][1]))
        report("synthesis: prog_depth_le(M, Max), fits(M, Ins, Labs)", rows)

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

    print("\n%d task(s) did not solve" % failures if failures else "\nall solved")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
