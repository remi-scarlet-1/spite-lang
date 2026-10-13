# Architecture Memo — Language / Runtime / Compiler

**Purpose:** fallback record of the proposed artifact structure. Not a spec itself — a
map of *what* artifacts exist and *why*, so a future session can resume without
re-deriving the reasoning.

**Status:** proposal (not yet fully adopted — runtime/compiler specs pending).

---

## 1. The three layers

A spite implementation is governed by three distinct bodies of decisions, and they
must stay in **separate artifacts**:

| Layer | Answers | Nature | Artifact |
| --- | --- | --- | --- |
| **Language** | what a spite program *means* | normative, timeless, implementation-independent | `rules.json` |
| **Runtime** | how a compiled program *executes on a host* | toolchain choice, platform-specific | `runtime-spec.md` *(to write)* |
| **Compiler** | how the reference implementation is *built and tested* | implementation choice, volatile | `compiler-spec.md` *(to write)* |

The governing principle from `rules-reference.md` holds here too: the *language*
slice is the only thing that belongs in `rules.json`. The runtime and compiler are
the *reference implementation's* choices, free to change without touching the
language.

## 2. Format distinction

- **`rules.json`** uses the **atomic-rule** format (`kind`, `statement`, `formal`,
  `inputs`, `outputs`, `depends_on`, `status`). That rigor is for *timeless
  normative semantics* — the thing you'd hand a second, independent compiler.
- **`runtime-spec.md` / `compiler-spec.md`** use **structured prose + precise
  decisions** (and, for the compiler, precise API signatures). They are *not*
  atomic rules — forcing implementation choices into `kind`/`formal` would be
  noise, and they must stay cheap to change.

## 3. `runtime-spec.md` — proposed contents

1. **Entry-point ABI** — `main` exposed as a C-callable symbol; its ABI-level
   signature; how the host invokes it.
2. **Exit code** — how `main`'s returned `i32` becomes the process exit status
   (low 8 bits? full value?), per `function.main.entry`.
3. **argv marshalling** — who allocates the backing `byte[][n]`, its lifetime, its
   encoding, how it's handed to `main`'s `byte[][]`.
4. **Memory model** — stack + `.rodata` only (no heap, since the rules define no
   allocation) — stated explicitly.
5. **Linking + calling convention** — standalone executable; libc? System V AMD64?
6. **Target platform** — which OS/arch first.

## 4. `compiler-spec.md` — proposed contents

1. **Pipeline** — lexer → parser → typecheck → codegen (llvmlite).
2. **Introspection API** — the Tier-3 test surface: `size_of(T)`, `align_of(T)`,
   `offset_of(T, f)`, `type_of(expr)`, `resolve(call)` — precise Python signatures
   the test harness calls.
3. **Test harness contract** — the `.spt` format, the kind→sign mapping, the three
   tiers (compile / runtime / introspection), and the runner protocol. This is the
   interface between the compiler agent and the test agent.
4. **CLI + diagnostics** — `spitec file.spt`; whether diagnostics cite rule IDs.

## 5. Ordering recommendation

**Runtime first**, then compiler. Rationale: `runtime-spec.md` is small and unblocks
the single most important observability question — *how does a test observe a
value?* (answer: `main`'s return → exit code, plus argv marshalling). Once that's
pinned, the compiler's test-harness contract has something concrete to point at,
and a test agent can actually write Tier-2 tests.

## 6. Already decided (in `rules.json` @ 0.1.1)

- `main` exists; signature `fn main(args: byte[][]) -> i32` (`function.main.entry`).
- exactly one `main` (`function.main.unique`).
- no `print` builtin; `main`'s return is the sole observable output.
- nested views legal, no depth limit, `&&T` still banned
  (`type.composition.nested-view`).

## 7. Open questions to adjudicate next (runtime)

1. **Entry symbol + exit code** — C-convention `int main(void)` and return `i32` →
   OS exit status (truncated to 8 bits)? *(proposed answer)*
2. **argv backing** — OS args copied into `.rodata`-resident `byte[][n]`; `args` is
   a read-only `byte[][]` view over it; lifetime = whole program. *(proposed answer)*
