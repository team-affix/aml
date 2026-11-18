# Continuous Documentation Guidelines

This file defines how future AI agents should behave when updating or extending the AML documentation. It exists because documentation errors are hard to spot and easy to introduce. Follow these rules strictly.

---

## Core principle: only document what was explicitly stated

Do not infer, interpret, or extrapolate. If something was not directly said, do not write it down. If you are unsure whether something is true, ask before writing.

This includes:
- Causal relationships ("X happens because Y") — ask unless stated.
- Sequencing ("X happens before Y") — ask unless stated.
- Method names, field names, entity names — ask if you did not see them used explicitly.
- Analogies to other systems (e.g. "this is like SAT-CDCL") — flag as your interpretation before including.

---

## Ask before writing

Before writing any non-trivial documentation, ask clarifying questions. If the answer leaves ambiguity, ask again. Do not resolve ambiguity yourself.

**Ask about:**
- What triggers an event or sequence of events.
- The exact ordering of steps.
- Whether a named entity/method actually exists in the codebase with that name.
- Whether a description covers all cases or just the common case.
- Whether something you are about to write is accurate or is just your best guess.

**Do not assume silence means agreement.** If the user did not confirm something, it is not confirmed.

---

## Precision in wording

Imprecise wording has caused repeated corrections. Specific patterns to avoid:

- **Do not conflate structural concepts.** Example: a "leaf resolution lineage" is the outermost resolution in the lineage tree — it does NOT mean a resolution against a fact. These are different things and must not be equated.
- **Do not say a single node "encodes a tree."** A single lineage node encodes one step. The tree emerges from the full set of chains.
- **Do not attribute a property to the wrong level of abstraction.** Example: "the solver is fully decoupled from construction order" was wrong — construction order still matters; what the resolver decouples is the constructor from knowing what its dependents are.
- **Do not compress event sequences into table cells.** If a row in a table describes what happens after an event fires, do not sneak in downstream consequences (e.g. "restart triggered") unless that is the precise and complete description.
- **Use the correct tense for event names.** Present participle (`-ing`) means in progress; past tense (`-ed`) means complete. This is a semantic contract, not a stylistic choice. Never swap them.

---

## Single source of truth

Every concept should be defined in exactly one place. Other files may reference it, but must not redefine or restate it.

- If a definition appears in multiple places, remove all but one and link to it.
- If a concept is closely related to another, ask where it belongs before placing it.
- Do not place a concept in a file just because it was mentioned in that context.

Examples of past misplacements that were corrected:
- `expr_pool` was in `lineage.md` — it belongs in `resolution.md`.
- `normalizer` was in `lineage.md` — it belongs in `resolution.md`.
- The leaf `resolution_lineage` definition was duplicated in `lineage.md` and `cdcl.md`.

---

## File organisation

Keep files focused. A file should cover one coherent topic. If a file is growing to cover multiple distinct concerns, consider splitting it — but ask before doing so.

When creating a new file:
- Add it to `overview.md`'s "Further reading" section.
- Link to it from any file that references its content.

---

## What to do when you are uncertain

1. Stop. Do not write.
2. Ask a specific, targeted question.
3. If the answer is clear, write. If there is still ambiguity, ask again.
4. Only after ambiguity is resolved, write the documentation.

This loop is not a formality. It has directly prevented incorrect documentation in this project multiple times.
