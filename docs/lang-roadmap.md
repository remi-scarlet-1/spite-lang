# Design Roadmap

Ordered by dependency, not by importance — an item is only placed after
everything it structurally relies on. Granularity is high near the top (single,
concrete decisions) and intentionally loosens further down, where the shape of
later work still depends on choices not yet made.

Legend: ✅ decided · 🔲 open

---

## Phase 0 — Already settled

✅ Variable mutability/initialization (`let`/`mut`,
defaulted/`undefined`/brace-init) ✅ References (non-null, `=` rebinds /
`*` and `->` deref, no `&&T`, dangling-ref scope limit) ✅ Structs (POD only,
mandatory interface list, methods vs namespaced fns) ✅ Ownership (move-only for
heap-owning types, explicit `move` — direction only; mechanics in Phase 4, item
12) ✅ Global allocator shape sketched (`alloc`/`free`, name-tracked) — not
finalized

## Phase 1 — Foundational language mechanics

_Nothing below this line can be fully designed without these._

1. ✅ **Built-in primitive types.** Confirm the full set and settle any that are
   still implicit rather than decided:
   - Numerics: `i8`..`i64`, `u8`..`u64`, `f32`/`f64` — already in use, worth
     confirming this is the complete list (`i128`/`u128`?). Still open.
   - 🔲 `usz` — the dedicated machine-word "index and size" type (`u64` on
     64-bit, `u32` on 32-bit). Deferred: for now `u64` stands in for array/slice
     indices, lengths, `.size`, and range loop counters. Promoting those to
     `usz` later is a pure rename.
   - ✅ `bool` — declared in the §1 primitive type table.
   - ✅ Character type — settled as `byte`. "Character" means one unit of
     opaque data, not a Unicode codepoint; text is a `byte[]` slice (§12).
2. ✅ **Casting/conversion rules.** Implicit numeric widening vs. explicit-only
   (`as`-style). Small, but referenced by nearly everything numeric.
3. ✅ **Sum types / tagged unions.** The mechanism `Optional<T>` and
   `Result<T,E>` both need underneath. Decide shape and pattern-matching syntax
   against it — depends on decision 1 (`switch`/`match` exhaustiveness).
4. ✅ **Fixed size arrays**
5. ✅ **Non-owning slice / view type — `(pointer, length)`, no ownership, no
   allocation.** More foundational than it first appears: it's already being
   relied on implicitly (`alloc(name: &byte[], ...)` is slice-shaped), and
   string literals are intended to decay into one referencing `.rodata` (item
   7). This is _not_ the same thing as the owning heap array (`Array<T>`, Phase
   3/4) — a slice never allocates or frees, it only views memory someone else
   owns. Settle the concrete syntax (`&T[]`? a distinct `Slice<T>`?) and how it
   relates to the existing fixed-size array syntax (`f32[3]`).
6. ✅ **Control flow.** `if`/`else` (§9), `while` (§10), `for` (§11), `switch`
   (§8). `if`/`match`/`switch` are expressions with explicit `yield`; loops and
   `for` are statements; `break`/`continue` bind to the innermost loop.
7. ✅ **String/text type.** Encoding is UTF-8 by assumption, not validation.
   String literals decay into a non-owning `byte[]` slice (item 5) referencing
   `.rodata` — no allocation for literals. There is no distinct character or
   string type, and no owning string: `byte[]` plus §4's slice operations is the
   whole of it. An _owning_, growable string, if one exists, is layered on top
   of the heap array (`Array<T>`) and deferred to Phase 3/4.

## Phase 2 — Error handling & control over failure

5. 🔲 **Recoverable errors.** `Result<T,E>`-shaped mechanism and its propagation
   operator (a `?`-equivalent). Depends on Phase 1, item 3 (sum types).
6. 🔲 **Unrecoverable errors.** Panic/assert/abort tier, distinct from (5) — for
   programmer-error conditions, not expected failure.

## Phase 3 — Data & polymorphism

7. 🔲 **Owning heap-array type (`Array<T>`).** The non-owning slice/view (Phase
   1, item 5) is settled by this point — this item is specifically the growable,
   allocator-backed container built on top of it. Directly blocks Memory
   Management (Phase 4).
8. 🔲 **Writable slice view (`mut T[]`).** §4.2 settles the read-only `T[]` view
   and the rule that `&mut` yields the writable `mut T[]`, but leaves the
   writable qualifier otherwise unspecified: whether a `mut T[]` coerces
   implicitly to `T[]` in argument position, and how a `mut T[]` field interacts
   with `mut self`. Writing through a slice of runtime length is what a
   buffer-filling function needs, so `Array<T>` (item 7) depends on this.
9. 🔲 **Polymorphism story — decide once, generally.** Generics/monomorphization
   only, an explicit interface/vtable construct, or both? This single decision
   is a prerequisite for: a generic `Mesh.create<A>(alloc: A, ...)`, the "any
   allocator" question in Phase 4, and the DLL protocol in Phase 6.
10. 🔲 **Generics — concrete syntax.** Declaration syntax, constraints/bounds,
    monomorphization rules. Depends on decision 9.

## Phase 4 — Memory Management (heap allocations)

_Where we left off before this detour — now properly sequenced after its real
dependencies._

11. 🔲 Finalize `alloc`/`free` signature (alignment param shape, failure
    handling — likely `Result`-shaped now that Phase 2 exists).
12. 🔲 **Move-only ownership, specified concretely.** Phase 0 settled the
    *direction* — a type that owns heap memory (`Array<T>`, item 7) is move-only:
    never copied (a copy would make two owners of one allocation → double-free),
    only *moved* with explicit `move` (`let b = move a;`, leaving `a`
    moved-from). Making this precise needs the thing being moved to already exist
    (`Array<T>`, item 7; `alloc`/`free`, item 11). Three sub-decisions:
    - **Cleanup model.** Reconcile §6's "no constructors, no destructors" with
      owning types: who frees the allocation when the owner dies? Manual free
      (`defer free(x)`, Zig-style — move then only prevents double-*use*) vs. an
      implicit drop/RAII (which would amend §6). Decide this first.
    - **Copyable vs. move-only classification.** A type is copyable iff it
      transitively owns no heap, move-only iff it transitively owns heap.
      Independent of the "view-holding" rule (§2.2): a struct holding `&mut` is
      still copyable, because views don't own.
    - **Move out of a `&mut`.** May `move` extract an owning field through a
      borrowed reference? It leaves a moved-from hole the real owner later frees →
      double-free. Likely forbid (or require writing a replacement back, Rust's
      `mem::replace`).
13. 🔲 Does an owning array/allocator carry its own allocator reference, or does
    the caller re-supply it at free time? (Depends on Phase 3, item 9 — whether
    "allocator" is a concrete type, a generic parameter, or an interface value.)
14. 🔲 User-defined allocators — shape and whether they share a common interface
    (same dependency as above).
15. 🔲 Concurrency, scoped narrowly: is the global tracked allocator required to
    be thread-safe? (Full concurrency model deferred — see Future.)

## Phase 5 — Compile-time machinery

16. 🔲 **Attribute/annotation syntax.** How a struct/function gets tagged (e.g.
    "this is a component," "this is exported") — small syntactic gap, but blocks
    Reflection.
17. 🔲 **Comptime.** What can run at compile time, what a comptime
    block/function looks like. Depends on Phase 1 (control flow) and likely
    Phase 3 (generics use comptime-like mechanics in most languages that have
    both).
18. 🔲 **Reflection.** Querying struct layout/tags/methods at comptime, and
    building the kind of registry described for the scene-loader example.
    Depends on 16 and 17.

## Phase 6 — Modules & the DLL protocol

19. 🔲 **Modules & visibility.** `module` namespacing already sketched —
    formalize visibility modifiers (public/private/internal-to-module?) and
    cross-file same-module resolution rules.
20. 🔲 **DLL runtime protocol.** Plugin spec files, hash-checked export binding,
    transparent call-through. The capstone feature — depends on Reflection (18,
    to generate the metadata block), Modules (19, spec files are presumably
    module-shaped), and Polymorphism (9, for calling an unknown-until-runtime
    function as if it were local).

---

## Future — loose, unordered, revisit later

- Full concurrency model (threading/job system, sync primitives, `alloc` under
  contention)
- Standard library scope (what ships vs. what's user-land)
- Build system / project structure across modules
- Testing story (built-in test blocks? external framework?)
- Tooling: formatter, LSP-relevant grammar concerns
- Interop beyond DLLs (C ABI boundary for third-party libs?)
- Opt-in `restrict`/`noalias` qualifier for `&mut` — revisit only if profiling
  demands the optimization headroom; the default is no alias assumptions (see
  reference §3.7)
- Function types / callables (first-class function values) — needed before the
  DLL runtime protocol (Phase 6) can call an unknown-until-runtime function
- Comparison semantics — whether `==` exists for slices, arrays, and structs,
  and what it means (element-wise? field-wise?). §12's "slice comparison" claim
  was removed as undefined
- Bit-level reinterpretation — narrow primitives (`as_bits`/`from_bits` for
  float↔int, plus a `byte` view of memory), NOT a general `reinterpret_cast`
