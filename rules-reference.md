# Rules Reference — interpreting `rules.json`

**Status:** live reference for `spec/rules.json`.
**Audience:** future agents, tooling authors, and compiler implementers.
**Purpose:** define how to read and interpret the ground-truth ruleset in
`spec/rules.json`. It documents the fixed layout, the field schema, the five-kind
taxonomy, the ID naming convention, and the notation used inside `formal`.

This document is *not* the extraction methodology. The process of deriving the
rules from `lang-reference-v2.md` is complete; that prose is **superseded**. The
rules are the source of truth: where a rule and the prose disagree, the rule wins.

---

## 1. What this is

`spec/rules.json` is the single, machine-readable, normative definition of the
language's semantics. It holds **242 atomic rules**. Each rule is one behavior —
one subject, one verb, one outcome — expressed as a JSON object with a fixed
schema (§3).

**Governing principle — no silent assumptions.** Every term a rule's `formal`
uses must itself be defined: either by the notation primitives (§6), or by another
rule named in that rule's `depends_on` (§8). A term with neither is a defect.

---

## 2. Layout (the tree)

`rules.json` is a JSON object whose keys nest **category → concept → … → rule**.
A rule's **ID is the dot-joined path** from the root to its leaf object.

```jsonc
{
  "memory": {                      // category
    "struct": {                    // concept
      "field": {                   // concept
        "offset": {                // the rule — ID: memory.struct.field.offset
          "kind": "definition",
          "statement": "…",
          "formal": "…",
          "inputs": "…",
          "outputs": "…",
          "depends_on": ["memory.struct.align", "type.primitive.size-align"],
          "status": "draft"
        }
      }
    }
  }
}
```

Rules live at **ragged depth**: most have three segments
(`category.concept.detail`), but a rule with only two segments
(`statement.break`, `match.expression`) is a leaf at depth two. There is no
artificial padding to force a uniform third level.

**Top-level keys** are `_info` (metadata — see §11) plus the **14 categories**:

```
array       enum        expression   function   lexical   match   memory
reference   slice       statement    string     struct    type    variable
```

The ID is never stored separately inside the object — it is always the dot-joined
path. `depends_on` references are these same full IDs.

---

## 3. Rule schema (the leaf object)

Every rule object has the fields below, in this order. Required fields are always
present and non-empty.

| Field | Required | Meaning |
| --- | --- | --- |
| `kind` | ✓ | One of the five kinds (§4). |
| `statement` | ✓ | One declarative sentence stating the behavior in plain language. |
| `formal` | ✓ | The rigorous content — algorithm, total function, recurrence, or decision table — in the notation of §6. This field carries the rigor. |
| `inputs` | ✓ | The domain the rule applies to and the preconditions under which it applies. |
| `outputs` | ✓ | The guaranteed result, or the error produced. |
| `depends_on` | ✓ | Array of rule IDs this rule uses. Empty for primitive/axiom rules. |
| `notes` | — | Non-normative commentary (rationale, examples). Never introduces new behavior. |
| `status` | ✓ | `draft`, `decided`, or `frozen` (§7). |

Fields removed from an earlier revision and no longer present: `src`, `gap`,
`supersedes`. Nothing in the corpus references them.

---

## 4. Kind taxonomy

The five kinds map one-to-one onto compiler behavior and test strategy. This is
what makes the spec executable rather than decorative.

| Kind | Meaning | Compiler must | Test asserts |
| --- | --- | --- | --- |
| `definition` | What the language *is* (layout, eval order, scope, name resolution). | Always emit/decide exactly this. | Positive: exact result. |
| `guarantee` | An invariant the compiler maintains. | Never emit a violating program. | Positive + a codegen/consistency assertion. |
| `constraint` | Input the compiler rejects. | Emit a diagnostic; reject. | **Negative**: the program must fail to compile. |
| `footgun.defined` | Accepted and unchecked, but *fully deterministic*. | Accept silently; behavior still specified. | Positive: exact result, no diagnostic. |
| `footgun.ub` | Accepted and unchecked; behavior **undefined**. | Accept; may do anything. | Assert *acceptance only*; never pin behavior. |

Two points that keep this rigorous:

1. **`footgun.defined` vs `footgun.ub` are not the same.** A rule like "aliasing
   operations apply in program order" is deterministic and testable, and is a
   `definition`-grade statement even though the language presents it as a
   "footgun." A rule like "division by zero" is UB and carries `unspecified`
   explicitly so no test pins it.

2. **Kind is a field, not part of the ID.** A rule may change kind (e.g. a
   footgun is later hardened into a constraint) without changing its ID.

---

## 5. ID naming convention

IDs are lower-case and dot-separated. **Every segment is a full word — no
abbreviations.** Within a segment, words are joined by hyphens
(`out-of-bounds`, `single-expression`).

The segments are `category.concept.detail` (or `category.concept` when there is
no third level). The category is always one of the 14 in §2.

**Established expansions** (the abbreviation → full word mappings that produced
the current names; use the full form when adding rules):

| Category | Concept / detail |
| --- | --- |
| `lex` → `lexical` | `decl` → `declaration` |
| `var` → `variable` | `init` → `initialization` |
| `mem` → `memory` | `impl` → `implementation` |
| `ref` → `reference` | `param` → `parameter` |
| `expr` → `expression` | `mut` → `mutable` |
| `stmt` → `statement` | `oob` → `out-of-bounds` |
| `fn` → `function` | `int` → `integer` |
| `str` → `string` | `const` → `constant` |
| | `div` → `division` |
| | `mod` → `modulo` |

**Terminology is normalized.** One term per concept, used everywhere:

- the thing a reference points to is always **`referent`** (never "pointee";
  "target" is reserved for the *cast target* and *assignment target*);
- reassigning a reference is always **`rebind`**.

---

## 6. Notation (primitives of the `formal` field)

These are the defined primitives a `formal` may use without further definition.
Anything not listed here must be a rule named in `depends_on`.

- `type(T)` — the type denoted by expression/declaration `T`.
- `size(T)`, `align(T)` — size/alignment in bytes of type `T`.
- `offset(fᵢ)` — byte offset of field/entity `i`.
- `align_up(x, a)` — the least multiple of `a` that is ≥ `x` (for `x ≥ 0`).
- `⟦e⟧` — the value (or l-value address) denoted by expression `e`.
- `f₀, f₁, …` — declaration-order indexing of fields.
- `⇒` — "evaluates to / reduces to."
- `err` — "this is a compile error (diagnostic)."
- `unspecified` — reserved for `footgun.ub`; asserts nothing about the outcome.
- Recurrence form: `x₀ = …; xᵢ = g(xᵢ₋₁, …)`.
- Unicode subscripts (`fᵢ`, `xᵢ₋₁`) and symbols (`≤`, `≥`, `≠`, `∈`, `∉`, `⊥`, `∞`) are used where they make the intent exact.

Further primitives are added by amending this section, never inline in a rule.

---

## 7. Status values

| Status | Meaning |
| --- | --- |
| `draft` | Grounded in the (now-superseded) prose; not yet locked. |
| `decided` | Encodes an explicit author decision (a point the prose left silent or ambiguous). |
| `frozen` | Locked — never silently edited; changes are versioned (§11). (None are `frozen`; the corpus is pre-1.0.) |

---

## 8. `depends_on` semantics

`depends_on` lists the **direct** dependencies only — the other rules whose
definitions this rule's `formal` uses. The graph is:

- **acyclic** (no rule transitively depends on itself), and
- **closed** (every referenced ID exists — no dangling edge).

Transitive reachability is implied and is *not* re-listed. An empty array marks an
axiom (a rule that depends only on notation primitives).

---

## 9. Normalization principles (the "sweet spot")

The corpus was normalized so that it stays both non-redundant and human-readable.
Four properties are maintained when the corpus changes:

1. **Atomic** — one rule states one behavior (it cannot be split into two rules
   that could be true/false independently, nor is it two behaviors that can never
   be observed apart).
2. **Deduplicated** — one fact appears in exactly one rule. Facts shared across
   constructs (e.g. "a condition must be `bool`") live once (in `statement.condition-bool`)
   and are referenced, not restated.
3. **Minimal `depends_on`** — direct dependencies only, nothing transitive or unused.
4. **Consistent terminology** — one term per concept (see §5).

Decomposition is deliberately **not** maximal: locality of behavior is preserved so
a rule can be read and cited (e.g. in a compiler diagnostic) without assembling ten
atoms.

---

## 10. Change control

- **Rules win.** On any conflict, the rules are authoritative; the superseded prose
  is never the tie-breaker.
- **Editing while pre-1.0.** At `0.x` (§11) the corpus is not yet stable: rules may
  be edited in place, and every normative edit is accompanied by a version bump.
- **Immutability once frozen.** At `1.0.0` the corpus freezes: a `frozen` rule is
  never silently edited, and any change is a versioned bump (§11) rather than an
  in-place edit.

---

## 11. Versioning

The corpus carries one semantic version, `MAJOR.MINOR.PATCH`, recorded in
`rules.json` under `_info.version`. A release bumps the *highest* component that
changed; the three positions mean:

| Bump | Meaning | Typical trigger |
| --- | --- | --- |
| **MAJOR** | **Breaking.** An existing rule's `kind`, `formal`, or `statement` changes, so a program that was accepted is now rejected, or an accepted program behaves differently. | a `footgun.ub` hardened into a `constraint`. |
| **MINOR** | **Non-breaking addition.** New rules are added, or wording is tightened without changing any accepted behavior. | a new operator or feature. |
| **PATCH** | **Non-normative.** Typos, `notes`, formatting — no rule's meaning changes. | a fixed subscript in a `formal`. |

Conventions:

1. A release bumps the highest component it touches (a new rule *and* a typo fix
   is MINOR, not PATCH).
2. MAJOR resets MINOR and PATCH to `0`; MINOR resets PATCH to `0`.
3. **Pre-1.0 (`0.x`).** The leading `0` means *not yet stable* — no stability
   contract is offered. During `0.x`, breaking changes and additions both bump
   MINOR (`0.1.0 → 0.2.0`); PATCH keeps its meaning (`0.1.0 → 0.1.1`). MAJOR is
   reserved for the `1.0.0` promotion.
4. **Promotion to `1.0.0`** is a deliberate act, taken once the rules have been
   validated (by tests and/or a compiler) and the author commits to their
   stability. From `1.0.0` on, MAJOR/MINOR/PATCH apply strictly.

---

## Reading a rule (quick guide)

To understand behavior `X`: locate its rule by dot-path in `rules.json`; read
`statement` for the plain meaning and `formal` for the exact construction; use
`kind` to know whether it is *defined*, *guaranteed*, *rejected* (`constraint`), or
*unspecified* (`footgun.ub`); follow `depends_on` to trace any term upward to its
definition.
