#!/usr/bin/env python3
"""Spell a dataset as spine terms for chc/spine.chc.

spine.chc wants data as an application spine -- data(emp) for the empty
list, app(app(data(cons), Head), Tail) for a cons, and a number n as n
nested app(data(suc), ...) around data(zero). Writing that by hand in a
terminal is miserable, so write the compact thing and let this print the
spine.

The spellings themselves are imported from bench_spine.py so that a goal
printed here is the same goal the benchmarks measure.

Two subcommands.

  term    one literal, printed as a spine term
  goal    a table of rows, printed as the fits/3 conjunct

Literals are ordinary Python, read with ast.literal_eval: True and False
become tags, an int becomes a suc spine, a list becomes a cons spine, and
a string passes through as a raw term for whatever has no compact
spelling -- a function, say, or a bare constructor.

Examples

  ./spine_data.py term 3
  ./spine_data.py term '[1, 2, 3]'
  ./spine_data.py term 'abs(var(zero))'
  ./spine_data.py goal '[([[]], 0), ([[1, 2]], 3), ([[1, 2, 3]], 6)]'
  ./spine_data.py goal '[([True, True], True), ([True, False], False)]'
  ./spine_data.py goal '[([[1]], [2])]' --max-nodes 3
  ./spine_data.py goal '[([[1]], [2])]' --max-depth 2
  ./spine_data.py goal '[(["abs(var(zero))"], True)]'

A row is (inputs, label) where inputs is a list, one element per argument
the program is applied to. No bound is printed unless asked for, because
choosing one is a decision worth making on purpose: fits applies the
candidate to the row itself, so the answer is usually point free and
shallower than the lambda you had in mind.
"""

from __future__ import annotations

import argparse
import ast
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from bench_spine import enc, peano  # noqa: E402  (needs the path above)


def table(rows: list) -> tuple:
    ins = "[" + ", ".join(
        "[" + ", ".join(enc(a) for a in args) + "]" for args, _ in rows) + "]"
    labs = "[" + ", ".join(enc(lab) for _, lab in rows) + "]"
    return ins, labs


def fits_goal(rows: list, var: str) -> str:
    ins, labs = table(rows)
    return "fits(%s, %s, %s)" % (var, ins, labs)


def read_literal(text: str):
    """A Python literal, or the text itself when it is already a term."""
    try:
        return ast.literal_eval(text)
    except (ValueError, SyntaxError):
        return text


def read_rows(text: str) -> list:
    try:
        rows = ast.literal_eval(text)
    except (ValueError, SyntaxError) as e:
        raise ValueError("%s\nrows must be a Python literal, e.g. "
                         "[([[1, 2]], 3)]" % e)
    if not isinstance(rows, (list, tuple)) or not rows:
        raise ValueError("rows must be a non-empty list of (inputs, label)")
    out = []
    for row in rows:
        if not isinstance(row, (list, tuple)) or len(row) != 2:
            raise ValueError("each row is (inputs, label), got %r" % (row,))
        args, label = row
        if not isinstance(args, (list, tuple)):
            raise ValueError("inputs must be a list, got %r -- a single "
                             "argument is still [x]" % (args,))
        out.append((list(args), label))
    return out


def main() -> int:
    ap_ = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap_.add_subparsers(dest="what", required=True)

    p_term = sub.add_parser("term", help="print one literal as a spine term")
    p_term.add_argument("literal")

    p_goal = sub.add_parser("goal", help="print a table as the fits/3 conjunct")
    p_goal.add_argument("rows")
    p_goal.add_argument("--var", default="M", help="name of the program variable")
    p_goal.add_argument("--max-depth", type=int, default=0,
                        help="prefix prog_depth_le with this nesting bound")
    p_goal.add_argument("--max-nodes", type=int, default=0,
                        help="prefix prog_size_le with this node bound")
    args = ap_.parse_args()

    if args.what == "term":
        print(enc(read_literal(args.literal)))
        return 0

    if args.max_depth and args.max_nodes:
        print("choose one of --max-depth and --max-nodes", file=sys.stderr)
        return 2
    try:
        rows = read_rows(args.rows)
    except ValueError as e:
        print(e, file=sys.stderr)
        return 2

    goal = fits_goal(rows, args.var)
    if args.max_depth:
        goal = "prog_depth_le(%s, %s), %s" % (args.var, peano(args.max_depth), goal)
    elif args.max_nodes:
        goal = "prog_size_le(%s, %s), %s" % (args.var, peano(args.max_nodes), goal)
    print(goal)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
