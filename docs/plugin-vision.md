# Plugin System Vision (memo — decision-driver, not spec)

**Purpose:** durable record of spite's binary/plugin north-star, so runtime/compiler
decisions stay aligned with it. This *drives* the specs; it is **not** spec content.

---

## Motivation (from C++ game-engine work)

Separating an engine runtime from developer game binaries (DLLs), with an ECS that
relies heavily on static typing. When a developer dynamically loads a DLL carrying
new ECS systems/components:

- the engine doesn't know the new types' memory layouts;
- multiple CRTs make heap management excruciating (alloc-in-one / free-in-another);
- there's no clear separation of each binary's capabilities;
- OS dynamic linking is unsafe (function-pointer casts, `reinterpret_cast`).

## Core insight

Static typing "evaporates at compile time." Spite's answer: **bake the type layouts,
function layouts, and their hashes into the binary**, so the typing survives to load
time and cross-binary compatibility becomes *verifiable*, not *assumed*.

## Three pillars

1. **Type-safe linkage** — a plugin's structural identity (layouts + signatures,
   hashed) is carried in the binary; the loader verifies it.
2. **Capability explicitness** — an API map declares exactly what a binary imports
   and exports; no implicit access.
3. **Host-owned memory** — one allocator (the host's), passed to plugins as a
   capability. No per-binary CRT.

## Mechanism

- The compiler builds **plugins** — spite dynamic libraries that the OS does *not*
  link. They contain foreign machine code + metadata (type/function layouts +
  hashes), and nothing else (no start symbol, no platform headers, no runtime).
- The compiler also emits a **second artifact (API map)** — machine-readable
  guarantees (exports: types, functions, hashes). A future host binary consumes it
  at *build time* and is statically checked against it.
- At *runtime*, the host loads a plugin and checks its **hash** against expectation.
  Match ⇒ complete, safe link. Mismatch ⇒ report which sections differ; trap if
  required.

## Two-layer guarantee

API map = **build-time** checking; load-time hash = **run-time** verification. Both
together ⇒ provable ABI compatibility (no casts, no layout surprises).

## Hard parts (flagged, solve later)

1. **Canonical, platform-stable hashing** (field order, padding, endianness must be
   canonicalized; identical across platforms/versions or "same hash" is meaningless).
2. **Transitive metadata completeness** (every type reachable from an export).
3. **Relocation-by-hash/name** (plugin references host symbols via a symbol table).
4. **Safety ≠ security** (the hash is a correctness guarantee, not a security
   boundary — a malicious plugin can lie).

## Where each piece lands (three-layer split)

| Piece | Layer |
| --- | --- |
| hash algorithm (what/order), layout rules, `export`/`import`/`plugin` constructs, capability model | Language (`rules.json`) |
| emit metadata + hashes + API map; emit relocatable PIC; consume API map to check a host | Compiler (`compiler-spec.md`) |
| loader: hash verify → resolve → link → trap; capability granting (host exports symbols) | Runtime (`runtime-spec.md`) |

## C-runtime consequence (decided direction)

- **Hosted + strictly static** — static libc (musl on Linux, static CRT on Windows).
- **Host owns the heap.**
- **No OS dynamic linking** — plugins are spite-managed, not OS-linked.
