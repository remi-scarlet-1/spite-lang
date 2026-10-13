# Compiler Specification (draft proposal)

**Status:** proposal — for review, not adopted.
**Relationship:** how the reference spite compiler is built and tested. Language
semantics live in `rules.json`; execution decisions in `runtime-spec.md`.

## 1. Pipeline

```
spite source → lexer → parser → typecheck → LLVM IR → object file → link → executable
              └────── spite's own frontend ──────┘   └ llvmlite ┘   └─ lld ─┘
```

Three stages: **frontend** (spite's own code), **codegen** (llvmlite), **link** (lld).
Spite builds LLVM IR directly — no C compiler, no C headers.

## 2. Toolchain

| Piece | Role | Notes |
| --- | --- | --- |
| llvmlite | build IR + emit object | bundles LLVM; its version pins the LLVM |
| lld | link object + CRT → executable | must match llvmlite's LLVM version |
| musl | static C library | per-target `.a` + `crt` artifacts |

## 3. Targets

- Primary: `x86_64-w64-windows-gnu` (musl CRT, static).
- Planned: `x86_64-pc-linux-musl` (static).
- Cross-compilation is inherent (swap target triple + musl artifact set).

## 4. Artifact store & self-containment

- **`toolchain.lock`** pins: llvmlite version, lld version (matching), musl version,
  target triples.
- **`assemble` script** downloads the pinned pieces (pip wheel, LLVM-release lld,
  musl artifacts). No building LLVM from source.
- **Self-contained, not minimal** — LLVM (~100–150 MB) is the accepted floor; no
  Clang shipped.
- **LLVM update** = bump llvmlite + lld as a matched pair (coupled to llvmlite's
  release cadence — a non-issue, since spite wants a *stable* backend).

## 5. Test harness contract (the compiler/test agent interface)

Three tiers keyed to what's observable:

| Tier | Surface | Observes | Format |
| --- | --- | --- | --- |
| 1 | compile | accept / reject | `.spt` + `// expect: compile-error` |
| 2 | runtime | `main`'s return (exit code) | `.spt` + `// expect: output "42"` |
| 3 | introspection | layout / type queries | Python tests calling the API (§6) |

**kind → sign mapping:**

| kind | test asserts |
| --- | --- |
| `constraint` | `// expect: compile-error` |
| `footgun.ub` | `// expect: compiles` (acceptance only; never pin behavior) |
| `definition` / `guarantee` / `footgun.defined` | `// expect: output …` (exact) |

Every test cites its rule ID (`// rule: <id>`), giving the rule→test coverage map.

## 6. Introspection API (Tier-3)

Python API the test harness calls: `size_of(T)`, `align_of(T)`, `offset_of(T, f)`,
`type_of(expr)`, `resolve(call)`.

## 7. CLI & diagnostics

- `spitec file.spt` → compile → executable.
- Diagnostics cite the violated rule ID.

## 8. Deferred (mapped — see `draft/plugin-vision.md`)

- Plugin format (type/function metadata + hashes).
- API map artifact (build-time interface contract).
- Relocatable PIC emission + relocation records.
- (The runtime loader itself lives in `runtime-spec.md` §7.)
