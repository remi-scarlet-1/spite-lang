# Runtime Specification (draft proposal)

**Status:** proposal — for review, not adopted.
**Relationship:** the runtime is the "how a compiled spite program executes on a
host" layer. Language semantics live in `rules.json`; this spec covers only
toolchain/execution decisions. The plugin-system north-star is a *decision-driver*
(see `draft/plugin-vision.md`), not specified here.

## 1. Hosting model

- **Freestanding, not hosted.** Spite does **not** link a C standard library.
- The runtime talks to the OS directly: the **Win32 API** (`kernel32.dll` /
  `ntdll.dll`) on Windows, raw **syscalls** on Linux. No musl, no ucrt, no glibc.
- **C interop is ABI-only** (calling convention + struct layout + an `extern`
  form); a libc is linked only if an interop *target* itself requires one —
  spite's own runtime never depends on it.
- **No OS dynamic linking** for spite code (no `ld.so` / DLL loading of spite).
  Plugins are spite-managed dynamic loading — deferred, mapped in `plugin-vision.md`.

## 2. Targets

- **Primary:** Windows x64 (mingw ABI — `x86_64-w64-windows-gnu`), freestanding.
- **Planned:** Linux x64, freestanding (raw syscalls).
- Cross-compilation is inherent: LLVM + lld target any triple, and a freestanding
  binary links no CRT at all.

## 3. Memory model

- Spite allocates memory **from the OS directly** — `VirtualAlloc` on Windows,
  `mmap` on Linux — **not** via a libc `malloc`.
- **Today:** stack + `.rodata` only. The language defines no heap allocation.
- The exact allocation *primitives* and the std suballocator design are still
  under discussion (deliberately left open here).
- **Future (stubs, all OS-direct):** threads (`CreateThread` / `clone`), I/O
  (`WriteFile` / `write`), C interop (ABI).

## 4. Entry point

```
OS loader → spite entry stub (PE/ELF entry) → spite main → OS exit
```

- **No crt0.** Spite is its own runtime startup.
- The binary's entry point (PE `AddressOfEntryPoint` / ELF `e_entry`) is a tiny
  spite-runtime stub that: fetches the command line (§5), marshals it into a
  `byte[][]`, calls spite's compiled `main(args: byte[][]) -> i32`, then exits (§6).
- This stub is freestanding runtime code, not a libc.

## 5. argv marshalling

- **Windows:** `GetCommandLineW` (kernel32) returns one UTF-16 string; the runtime
  parses it into arguments and converts each to UTF-8 `byte[]` slices.
- **Linux:** the kernel places `argc`/`argv` on the initial stack; the entry stub
  reads them directly.
- The `byte[][n]` backing array is **stack-resident** in the entry stub; lifetime =
  whole program.
- This is per-OS code in spite's runtime — *not* uniform; the uniform `byte[][]`
  is what spite's runtime *produces* out of the OS-specific input.

## 6. Exit code

- spite's `main` returns `i32` → the entry stub calls `ExitProcess(code)` (Windows)
  or the `exit` syscall (Linux).
- Windows: full 32-bit value. Linux: low 8 bits.

## 7. Future sections (stubs — mapped, not specified)

| Section | Direction |
| --- | --- |
| Heap | OS-direct (`VirtualAlloc` / `mmap`); suballocators live in spite's std |
| Threads | OS-direct (`CreateThread` / `clone`) |
| I/O | OS-direct (`WriteFile` / `write`) — spite has no `print` builtin today |
| FFI | C ABI; link C libraries only if an interop target needs them |
| Plugin loader | spite-managed dynamic loading — see `draft/plugin-vision.md` |
