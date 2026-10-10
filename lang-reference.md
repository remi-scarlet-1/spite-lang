# Language Reference

This reference uses three callouts:

- **Guarantee** — an invariant the compiler maintains: it will not emit a
  program that violates it, so you can rely on it.
- **Constraint** — a restriction the compiler enforces: code that violates it is
  rejected (a compile error, or a warning where noted).
- **Footgun** — neither maintained nor enforced: the compiler accepts the code,
  and the risk is a known, accepted one you carry.

## Contents

- [Lexical conventions](#lexical-conventions)
- [1. Primitive Types](#primitive-types)
  - [1.1 Casting](#casting)
- [2. Variables](#variables)
  - [2.1 Mutability](#mutability)
  - [2.2 Initialization](#initialization)
  - [2.3 Type Inference](#type-inference)
- [3. References](#references)
  - [3.1 Non-nullability](#non-nullability)
  - [3.2 Mutability of the pointee or view](#mutability-pointee)
  - [3.3 Assignment, dereferencing, and member access](#dereferencing)
  - [3.4 Reference-typed struct fields](#reference-fields)
  - [3.5 Disallowed indirection](#disallowed-indirection)
  - [3.6 Dangling references](#dangling-references)
  - [3.7 Aliasing](#aliasing)
- [4. Arrays & Slices](#arrays-and-slices)
  - [4.1 Fixed-size arrays](#fixed-arrays)
  - [4.2 Slices](#slices)
  - [4.3 Sub-slicing](#sub-slicing)
- [5. Functions](#functions)
  - [5.1 Declaration](#function-declaration)
  - [5.2 Parameters](#parameters)
- [6. Structs](#structs)
  - [6.1 Declaration](#struct-declaration)
  - [6.2 Implementation](#struct-implementation)
  - [6.3 Methods vs. namespaced functions](#methods)
- [7. Enums (sum types / tagged unions)](#enums)
  - [7.1 Declaration](#enum-declaration)
  - [7.2 Instantiation](#enum-instantiation)
  - [7.3 Reference-payload variants](#reference-payload-variants)
  - [7.4 Pattern matching](#pattern-matching)
- [8. Switch](#switch)
- [9. If / Else](#if-else)
- [10. While](#while)
- [11. For](#for)
- [12. Strings](#strings)

---

<a id="lexical-conventions"></a>
## Lexical conventions

Comments start with `//` and run to the end of the line, or are delimited by
`/* */`. They may appear anywhere whitespace may and are ignored.

An unsuffixed integer literal is `i32` by default, or the context's integer
type when one is present (`let y: i64 = 42;`).

An unsuffixed float literal is `f32` by default, or the context's float type
when one is present.

Integer literals are decimal, hexadecimal (`0x…`), or binary (`0b…`), with `_`
allowed as a separator (`1_000_000`). There is no octal form and no type suffix
on literals.

The keywords are:

`let mut fn struct enum match switch if else while for in yield break continue return as true false undefined`

The primitive type names — `bool i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 byte` — are reserved and cannot be shadowed, but are not keywords; they are type names, resolved like any other type.

Operator precedence, tightest first:

1. unary `-`, `!`, `~`, `&`, `*`
2. `as`
3. `*`, `/`, `%`
4. `+`, `-`
5. `&`, `^`, `|`
6. `<`, `<=`, `>`, `>=`
7. `==`, `!=`
8. `&&`
9. `||`

Bitwise operators bind tighter than comparisons, unlike C (`a & b == 0` is
`(a & b) == 0`).

Compound assignment operators are `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`,
`^=`. Each is `x = x op rhs` and, like `=`, is a statement.

---

<a id="primitive-types"></a>
## 1. Primitive Types

| Type                      | Example literal | Arithmetic              |
| -------------------------- | ---------------- | ------------------------ |
| `bool`                    | `true`, `false`  | none — logical (`&&`, `\|\|`, `!`) and comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) operators |
| `i8`, `i16`, `i32`, `i64` | `42`             | full                     |
| `u8`, `u16`, `u32`, `u64` | `42`             | full                     |
| `f32`, `f64`             | `1.0`            | full, except `%`         |
| `byte`                    | `'x'`            | none — equality/ordering (`==`, `!=`, `<`, `>`, `<=`, `>=`) and bitwise (`&`, `\|`, `^`, `~`) |

`byte` represents one unit of opaque, unmarked data (full 0–255 range) — not
a number, not necessarily text. A `byte` literal is written `'x'`, reusing the
character-literal syntax; a `byte` is not a character type, and the syntax is
shared purely because both denote one unit of data.

A `byte` literal supports the same escape sequences as a string literal
([§12](#strings)): `\0`, `\n`, `\r`, `\t`, `\\`, `\"`, `\'`, and `\xNN` for a
single byte by two-digit hex value.

Because `\xNN` reaches the full 0–255 range, a `byte` need not come from a cast
or a data-producing API: 128–255 are written `'\x80'`–`'\xFF'`.

> **Constraint** — the non-escaped `'x'` form is restricted to ASCII (0–127); a
> literal like `'é'` is a compile error.

The restriction exists because a character literal names one unit of data, and
which bytes an above-ASCII character occupies would depend on an encoding the
source file is not assumed to carry. Above-ASCII bytes are written in `\xNN`
form instead.

Bitwise operators (`&`, `|`, `^`, `~`) work on `byte` — bit manipulation, not arithmetic. Shifts are not yet supported.

> **Guarantee** — no primitive type converts to another implicitly.

Primitive sizes and alignments, in bytes: `i8`/`u8`/`byte`/`bool` = 1,
`i16`/`u16` = 2, `i32`/`u32`/`f32` = 4, `i64`/`u64`/`f64` = 8. A
primitive's alignment equals its size.

> **Guarantee** — integer arithmetic wraps on overflow (two's complement).
> Overflow is neither an error nor undefined behavior.

> **Footgun** — division and modulo by zero are undefined behavior, as in C. No
> check is emitted; the risk is accepted and carried by the programmer.

Signed division and modulo truncate toward zero (`-7 / 2 == -3`, `-7 % 2 == -1`),
matching C.

Floating-point arithmetic is IEEE 754, round-to-nearest, with no fast-math
relaxations. NaN compares unequal to everything, including itself (`NaN == NaN`
is `false`).

> **Constraint** — `%` is defined on integer operands only. Modulo is
> ill-defined on floats, so `f32 % f32` and `f64 % f64` are compile errors.

<a id="casting"></a>
### 1.1 Casting

```rs
let x: i64 = 1000;
let y: i32 = x as i32;
```

`as` converts between primitive types, in either direction (widening or
narrowing). Conversion is never implicit.

> **Constraint** — a narrowing/truncating cast (e.g. `i64 as i32`, `f64 as f32`,
> `i32 as u8`) produces a compiler warning.

> **Constraint** — `as` does not apply to structs — there is no struct-to-struct
> conversion. Bit-level reinterpretation (e.g. `f32`'s raw bits as `u32`) and
> extracting an enum variant's tag are separate, not-yet-specified operations,
> and are not part of `as`.

> **Constraint** — `as` to or from `bool` is a compile error. `as` between `byte`
> and `u8` is allowed in either direction.

---

<a id="variables"></a>
## 2. Variables

<a id="mutability"></a>
### 2.1 Mutability

```rs
let t: Transform; // constant l-value — cannot be reassigned
mut t: Transform; // mutable l-value — can be reassigned
```

Bindings are scoped to their enclosing block; an inner binding may shadow an
outer one of the same name.

<a id="initialization"></a>
### 2.2 Initialization

```rs
let t: Transform;                             // (a) defaulted
let t: Transform = undefined;                 // (b) uninitialized
let t: Transform = Transform{ .pos_x = 1.0 }; // (c) partial, rest defaulted
```

| Form            | Unset fields                                     |
| --------------- | ------------------------------------------------ |
| (a) defaulted   | zeroed, or per-field default if declared ([§6.1](#struct-declaration))         |
| (b) `undefined` | garbage — whatever bits were already there       |
| (c) brace-init  | zeroed / per-field default, except fields listed |

> **Constraint** — a struct with a reference-typed field
> ([§3.4](#reference-fields)) cannot use form (a), because a reference field
> cannot declare a default ([§6.1](#struct-declaration)). Form (c) must supply
> every reference field explicitly.

> **Constraint** — form (b) `undefined` is legal only for a *value type*: a type
> whose layout, transitively, contains no reference ([§3](#references)) and no
> slice ([§4.2](#slices)). A struct holding a reference or slice — directly, or
> nested inside another struct — cannot be `undefined`-initialized.

```rs
struct Point { x: f32; y: f32; }       // value struct — no view fields
struct Wrapper { inner: &mut i32; }    // view-holding struct

let n: u32 = undefined;                // OK — value type
let buf: byte[256] = undefined;        // OK — value type, no view
let p: Point = undefined;              // OK — value struct

let r: &u32 = undefined;               // ERROR — a reference is a view
let s: f32[] = undefined;              // ERROR — a slice is a view
let w: Wrapper = undefined;            // ERROR — holds a reference transitively
```

<a id="type-inference"></a>
### 2.3 Type Inference

```rs
let t: Transform = Transform{ .pos_x = 1.0 };
let t            = Transform{ .pos_x = 1.0 }; // equivalent
```

---

<a id="references"></a>
## 3. References

A reference (`&T`) is a non-nullable indirection to a value — one machine word
(a pointer).

| Position                        | Meaning                    | Example  |
| ------------------------------- | -------------------------- | -------- |
| Before a type                   | forms a reference type     | `&T`     |
| Before a type                   | forms a mutable reference type | `&mut T` |
| Before an l-value (unary)       | address-of                 | `&x`     |
| Before an array-l-value (unary) | read-only view of it       | `&a`     |
| Before an array-l-value (unary) | writable view of it        | `&mut a` |
| Between two operands (binary)   | bitwise AND                | `a & b`  |

`&` and `&mut` follow the same read-only/writable split whether the operand is
an ordinary value (yielding a reference, [§3.2](#mutability-pointee)) or an
array-like l-value (yielding a slice, [§4.2](#slices)).

Access through a reference is explicit: `*` dereferences to the whole referent
and `->` dereferences for member access ([§3.3](#dereferencing)).

<a id="non-nullability"></a>
### 3.1 Non-nullability

> **Guarantee** — a `&T` is never null and is always bound to a live value at
> the moment it is created. There is no syntax that produces a null reference,
> and `undefined` cannot initialize a reference or any reference-holding value
> ([§2.2](#initialization)), so a non-live reference cannot be *created*. For a
> value that may legitimately be absent, wrap it in a sum type ([§7](#enums)) instead of relaxing
> this guarantee.
>
> This guarantee is about *creation*, not the target's lifetime: a reference can
> still outlive its referent and become dangling, which is unchecked
> ([§3.6](#dangling-references) — a footgun, not a guarantee).

<a id="mutability-pointee"></a>
### 3.2 Mutability of the pointee or view

```rs
&T       // read-only view
&mut T   // write access
T[]      // read-only slice view (§4.2)
mut T[]  // writable slice view (§4.2)
```

> **Constraint** — taking `&mut` of a value requires that value's own binding be
> `mut`. The same holds for `&mut` on an array-like l-value, and for slice types:
> writes through a `T[]` are rejected regardless of the binding's mutability,
> because `T[]` is a read-only view type ([§4.2](#slices)).

<a id="dereferencing"></a>
### 3.3 Assignment, dereferencing, and member access

A reference is a value of type `&T` (or `&mut T`) that holds an address. `=`
assigns a value of a variable's own type to that variable. For a reference
variable that value is an address, so `=` *rebinds* it — it never writes through:

> **Constraint** — `=` is a statement, not an expression: it produces no value
> and may not appear where a value is expected.

```rs
mut a: u64 = 42;
mut b: &u64 = &a;   // b holds a's address

mut c: u64 = 99;
b = &c;             // rebind — b now holds c's address; a is unaffected
```

> **Constraint** — the right-hand side of a rebind must match the reference's type
> and mutability: a `&mut u64` is assigned `&mut u64`, never `&u64`
> ([§3.2](#mutability-pointee)).

Writing through a reference is an explicit dereference. `*` turns a reference
into an l-value naming its referent:

```rs
mut d: &mut u64 = &a;
*d = 10;            // write through — a is now 10
```

Writing through `*` requires `&mut`, exactly as [§3.2](#mutability-pointee) says;
`*` on a `&T` reads but cannot write.

Member access is explicit in the same way. `.` accesses a member at the
receiver's own indirection level, while `->` dereferences one level and then
accesses the member:

```rs
struct Point { x: f32; y: f32; }

mut p: Point = undefined;
let r: &mut Point = &p;

r->x = 1.0;              // deref r, then write p.x
let px: &mut f32 = r.x;  // `.` projects the reference — a &mut to the field
```

On a value, `.` gives the member value itself (`p.x` is a `f32` l-value); on a
reference, `.` gives a reference to the member of the same mutability (`r.x` is
a `&mut f32`). `->` always gives the member value and is only meaningful on a
reference.

> **Constraint** — compound assignment (`+=`, `-=`, …) is defined as `x = x op
> rhs`, so it rebinds a reference rather than writing through it. To write
> through, dereference first: `*d += 1`, not `d += 1` (an error — an address has
> no arithmetic).

Each operator therefore has exactly one meaning:

| Operator | Meaning                                           |
| -------- | ------------------------------------------------- |
| `=`      | assign a value of the variable's own type         |
| `*`      | dereference — the referent as an l-value          |
| `->`     | dereference, then member access                   |
| `.`      | member access at the receiver's indirection level |

<a id="reference-fields"></a>
### 3.4 Reference-typed struct fields

> **Constraint** — a struct with a reference-typed field cannot be
> default-initialized ([§2.2](#initialization), form (a)), because a reference
> field cannot declare a default ([§6.1](#struct-declaration)); and it cannot be
> `undefined`-initialized (form (b)), because `undefined` is limited to value
> types ([§2.2](#initialization)). Full brace-init, supplying a real target for
> each reference field, is required.

```rs
struct Handle {
    target: &mut Entity;
}

let h: Handle;                          // ERROR — no default for `target`
let h: Handle = undefined;              // ERROR — `undefined` cannot hold a view (§2.2)
let h: Handle = Handle{ .target = &e }; // OK
```

<a id="disallowed-indirection"></a>
### 3.5 Disallowed indirection

> **Constraint** — `&&T` is not a legal type.

<a id="dangling-references"></a>
### 3.6 Dangling references

A reference can outlive its referent — e.g. a struct holding a reference field
that outlives the stack-local value it points to:

```rs
struct Holder { r: &mut i64; }

fn MakeHolder() -> Holder {
    mut local: i64 = 5;
    return Holder{ .r = &local }; // local's storage ends when makeHolder returns
}
```

> **Constraint** — a function may not directly `return &local;` or
> `return &mut local;` where `local` is owned by that function's own stack frame.

> **Footgun** — no other dangling-reference case is checked — e.g. a local's
> address smuggled out inside a struct field (as above), or a reference stored
> somewhere and outliving its target through any more indirect path.

<a id="aliasing"></a>
### 3.7 Aliasing

A reference's mutability says what *one* reference may do; it says nothing about
what *other* references exist. `&T` is an enforced guarantee — the compiler
rejects any write through it ([§3.2](#mutability-pointee)). `&mut T` is a
*signal* — "this reference may write" — and carries no exclusivity: two `&mut`
to the same memory, or a `&T` and a `&mut` to the same memory, are both legal.

That `&T` guarantee is narrower than it looks: it forbids writes *through that
reference*, not writes *to the target by someone else*. A coexisting `&mut` can
change the value under a `&T`; `&T` promises its holder cannot be the one to
change it, not that the target stays stable.

Aliasing is the complementary hazard to [§3.6](#dangling-references): dangling
is a reference outliving its target, aliasing is two references reaching the
same target. Both are unchecked, but they fail differently — dangling tends to
crash loudly; aliasing corrupts data silently.

> **Guarantee** — aliasing is *well-defined*. The compiler makes no assumption
> that a `&mut` is the only reference to its target (no `noalias`/`restrict`
> semantics). A program's observable behavior is deterministic: operations apply
> in program order, whether or not two references happen to alias.

> **Footgun** — nothing enforces exclusivity, so two writers to the same memory
> produce a deterministic but almost-certainly-unintended result. The compiler
> cannot diagnose it; the corruption shows up as a wrong value with no obvious
> origin.

The language relies on discipline rather than a borrow checker. Three
conventions keep aliasing tractable in practice:

1. **Long-lived handles are indices, not references.** A reference that outlives
   one function call — stored in a struct field or an enum payload, held across
   frames — should instead be an index into the owning array or arena, looked up
   at each use. An index re-borrows from the single owner on every access, so it
   cannot create aliasing.
2. **`&mut` is transient.** Take it for the duration of a function (a parameter
   or a local), where its scope is small enough to review for aliasing by eye.
3. **Single-writer convention.** At any moment a piece of memory has at most one
   writer — the holder of its `&mut`. Many readers (`&T`) may coexist; the
   `&`/`&mut` split already enforces the reader half for you
   ([§3.2](#mutability-pointee)).

---

<a id="arrays-and-slices"></a>
## 4. Arrays & Slices

<a id="fixed-arrays"></a>
### 4.1 Fixed-size arrays

```rs
let a: f32[3] = [1.0, 2.0, 3.0];
```

A fixed-size array's length is part of its type — `f32[3]` and `f32[4]` are
distinct types. It is a plain value type: assigning one to another, or passing
it by value, copies every element.

`arr[index]` is a dedicated language mechanism, not sugar for pointer arithmetic
— the compiler computes the element's address internally, but this is never
exposed to or performed by user code.

> **Footgun** — out-of-bounds indexing is not checked at runtime.

`arr.size` gives the array's length — a compile-time constant, since the length
is part of the type. Indices and `.size` are `u64`, and `.size` is a read-only
builtin, not a declarable field.

<a id="slices"></a>
### 4.2 Slices

```rs
let s: f32[] = &a; // a slice viewing all of a
```

A slice (`T[]`) is a non-owning, runtime-sized view — a `{ T*, u64 }` pair
(16 bytes on 64-bit). `&arr` on a fixed-size
array produces one — reusing `&` in its usual role of obtaining a non-null view
of a value ([§3](#references)), but the result is a `T[]`, not a `&T`.

`&a` and `&mut a` on a fixed-size array produce slices, not `&T` and `&mut T`:
`&a` yields a read-only `T[]`, and `&mut a` yields a writable `mut T[]`. Neither
is address-of-taking in the [§3.2](#mutability-pointee) sense.

> **Constraint** — `&mut a` requires `a`'s own binding to be `mut`, exactly as it
> does for a reference ([§3.2](#mutability-pointee)).

A slice type carries its own writability, like a reference does: `T[]` is a
read-only view and `mut T[]` is a writable one.

> **Constraint** — writing an element through a read-only view is a compile
> error.

> **Guarantee** — because this writability is part of the type, it holds across a
> function boundary: a callee receiving a `T[]` cannot write through it
> regardless of what the caller owns.

Binding mutability is a separate axis, and `=` on a slice rebinds it the same way
it rebinds any value ([§3.3](#dereferencing)). It governs whether the slice
binding may be reassigned, not whether its elements may be written: `let v: mut
f32[] = &mut a;` may write `v[0]` but may not reassign `v`; `mut v: f32[] = &a;`
may reassign `v` but may not write `v[0]`.

Sub-slicing ([§4.3](#sub-slicing)) preserves the qualifier — a `mut T[]`
sub-slice is a `mut T[]`, and a `T[]` sub-slice is a `T[]`.

> **Guarantee** — a read-only view can never be widened into a writable one; the
> reverse is free.

`slice[index]` and sub-slicing ([§4.3](#sub-slicing)) are dedicated language
mechanisms, implemented internally by the compiler computing element addresses —
the same treatment array indexing already gets ([§4.1](#fixed-arrays)). This is
compiler-internal codegen, not a capability exposed to user code: a slice's
internal base value is not a `&T` and is not subject to [§3](#references)'s
reference rules (it cannot be repointed directly, and is never itself an
arithmetic-capable value in user code).

`slice.size` gives the slice's current length — a load of the slice's length
word, the same read-only builtin as `arr.size` ([§4.1](#fixed-arrays)).

A slice does not own the memory it views — its validity depends entirely on the
underlying data outliving it, subject to the same dangling-reference risk
described in [§3.6](#dangling-references).

> **Constraint** — a slice is a view, so it cannot be `undefined`-initialized
> ([§2.2](#initialization)). A slice comes into existence only by viewing a real
> array (`&a`/`&mut a`), by sub-slicing ([§4.3](#sub-slicing)), or from a string
> literal ([§12](#strings)).

<a id="sub-slicing"></a>
### 4.3 Sub-slicing

```rs
let mid: f32[] = a[1..3];   // elements 1 and 2 — `end` is exclusive
let thru: f32[] = a[1..=3]; // elements 1, 2, 3 — `end` is inclusive
```

Indexing with a range instead of a single value produces a sub-slice covering
that range, rather than a single element. A range is written `a..b` (exclusive
of `b`) or `a..=b` (inclusive of `b`); the same two forms appear everywhere a
range does ([§8](#switch), [§11](#for)).

---

<a id="functions"></a>
## 5. Functions

<a id="function-declaration"></a>
### 5.1 Declaration

```rs
fn MyFunction();                              // no return value
fn MyFunction() -> ReturnType;                // returns ReturnType
fn MyFunction(x: i32, y: i32) -> ReturnType;  // with parameters
```

A function may be called before its declaration appears in the file; names are
resolved across the whole program.

> **Constraint** — a function declared with a return type must `return` a value
> on every path that reaches its closing brace. Reaching the end of a `-> T`
> function body without executing `return` is a compile error.

<a id="parameters"></a>
### 5.2 Parameters

A parameter is an immutable binding ([§2.1](#mutability)): it cannot be
reassigned.

> **Constraint** — reassigning a parameter is a compile error. Mutability is
> expressed only through the parameter's *type*: a `&mut T` parameter is a
> mutable reference whose pointee may be written ([§3.2](#mutability-pointee)),
> but the parameter binding itself — the reference — remains immutable and
> cannot be repointed. To rebind a value, copy the parameter into a `mut` local
> first.

A `&mut` parameter is a reference; writing through it is an explicit
dereference ([§3.3](#dereferencing)):

```rs
fn Increment(v: &mut i32) {
    *v = *v + 1; // write through the reference
}
```

---

<a id="structs"></a>
## 6. Structs

Structs are plain data: no classes, no constructors, no destructors.

<a id="struct-declaration"></a>
### 6.1 Declaration

A struct's body lists its fields, followed by the signatures of any functions
namespaced onto the type.

> **Constraint** — this signature list is the struct's complete interface — a
> function may be namespaced onto a type only if its signature is declared here.

```rs
struct Transform {
    pos_x: f32;
    pos_y: f32;
    rotation: f32;

    fn Identity() -> Transform;
    fn Translate(self: &mut Transform, dx: f32, dy: f32);
};
```

Struct fields are laid out in declaration order, each at its natural alignment,
with padding inserted as needed. A struct's size is rounded up to a multiple of
its alignment, which is the largest of its fields'.

A field may declare a default value, used when the field is otherwise
uninitialized ([§2.2](#initialization)):

```rs
struct Point {
    x: f32 = 0.0;   // default 0.0
    y: f32 = 0.0;
}
```

The default must be a compile-time constant and may appear only on value
fields — a reference-typed field cannot declare a default
([§3.4](#reference-fields)).

<a id="struct-implementation"></a>
### 6.2 Implementation

Implementations are written separately, qualified with the struct's name, and
may appear anywhere in the program:

```rs
fn Transform.Identity() -> Transform {
    mut t: Transform = undefined;
    t.pos_x = 0.0;
    t.pos_y = 0.0;
    t.rotation = 0.0;
    return t;
}

fn Transform.Translate(self: &mut Transform, dx: f32, dy: f32) {
    self->pos_x += dx;
    self->pos_y += dy;
}
```

<a id="methods"></a>
### 6.3 Methods vs. namespaced functions

A function whose **first** parameter is a reference to its own struct type
(`&Type` or `&mut Type`) is a **method**. `self` is a naming convention, not a
keyword.

```rs
my_transform.Translate(1, 1);
// equivalent to:
Transform.Translate(&my_transform, 1, 1);
```

`&mut Type` self-parameters require the instance's own binding to be `mut`.

Calling a method on a reference uses `->`, as with field access
([§3.3](#dereferencing)): where `r: &mut Transform`, `r->Translate(1, 1)` is
`Transform.Translate(r, 1, 1)`.

A function declared without a leading self-parameter is namespaced to the type
but is not a method, and is called as `Transform.identity()`.

---

<a id="enums"></a>
## 7. Enums (sum types / tagged unions)

An enum holds exactly one of several named variants at a time. In memory it is
a tag at offset 0, then padding, then the active variant's payload. The payload
area is sized and aligned to the largest variant, and the enum's total size is
rounded up to that alignment.

<a id="enum-declaration"></a>
### 7.1 Declaration

```rs
enum Event: u8 {
    NoOp,
    MousePos(xDelta: f32, yDelta: f32),
    MouseClick(code: u8),
}
```

> **Constraint** — the tag type is mandatory and explicit — `enum Name: TagType`.
> There is no auto-inference from variant count.

> **Constraint** — a variant with no data omits the parameter list — `NoOp,` not
> `NoOp(),`.

Variant payloads are declared using the same syntax as function parameter lists
([§5.1](#function-declaration)).

<a id="enum-instantiation"></a>
### 7.2 Instantiation

A value is constructed by qualifying the variant name with its enum, passing
field values positionally in declared order:

```rs
let e: Event = Event.MousePos(1.0, 2.0);
let n: Event = Event.NoOp;
```

<a id="reference-payload-variants"></a>
### 7.3 Reference-payload variants

A variant may hold a reference to a larger payload instead of the payload
inline, to avoid inflating every instance to the size of its largest variant:

```rs
enum Event: u32 {
    MouseMove(p: &MouseMovePayload),
    MouseClick(p: &MouseClickPayload),
}
```

This is manual — the language never boxes a variant automatically.

> **Footgun** — a reference-holding variant is subject to the same risk as any
> other reference ([§3.6](#dangling-references)): it can outlive its referent.

<a id="pattern-matching"></a>
### 7.4 Pattern matching

```rs
let result = match (event) {
    Event.NoOp => 0,
    Event.MousePos(dx, dy) => {
        let dist: f32 = sqrt(dx * dx + dy * dy);
        yield dist;
    },
    Event.MouseClick(code) => HandleClick(code),
};
```

`match` is an expression. A single-expression arm's value is that expression; a
block-bodied arm produces its value via an explicit `yield expr;` statement,
which exits the block immediately, the same way `return` exits a function. When
a `match`'s result is discarded, block-bodied arms need not `yield` and run as
an ordinary statement sequence.

> **Constraint** — when used as an expression, every arm must produce a value of the same type.

> **Constraint** — `match` must handle every variant of the enum being matched.

An enum's variant set is closed (fixed at declaration), so listing every variant
by name satisfies this on its own. `else` may be used as a final arm to cover
the rest:

```rs
let label: byte = match (event) {
    Event.NoOp => 'n',
    else => 'x',
};
```

> **Guarantee** — arms are checked in order, and no fallthrough occurs — exactly
> one arm's expression is evaluated.

Payload destructuring is positional, matching the variant's declared field order
([§7.2](#enum-instantiation)) — the same convention used for construction and
function calls. A field that isn't needed can simply be bound and left unused.

---

<a id="switch"></a>
## 8. Switch

`switch` matches a value against primitive literals, multiple discrete values,
or ranges. It is a sibling to `match` ([§7.4](#pattern-matching)) — same arm
syntax, same expression/`yield` semantics, same no-fallthrough guarantee —
differing only where it structurally must: a `switch`'s value domain (integers,
`byte`, etc.) is open, not closed like an enum's variant set, so it cannot be
exhaustive by listing cases alone.

```rs
let name: byte = switch (code) {
    1 => 'a',
    2, 3 => 'b',
    10..20 => 'c',
    else => '?',
};
```

> **Constraint** — a final `else` arm is mandatory; omitting it is a compile
> error.

An arm may list multiple comma-separated values, matching if the subject equals
any of them.

The subject's type determines the literal form its cases are written in — an
integer subject switches on integer literals, a `byte` subject on `byte`
literals.

> **Constraint** — a case cites one or more literals, not expressions.

A case may mix single values and ranges freely; each is checked as its own
match.

> **Constraint** — a range case's endpoints are literals, not expressions.

An arm may specify a range, matching if the subject falls within it. `a..b`
matches from `a` inclusive to `b` exclusive; `a..=b` matches from `a` through
`b` inclusive. The inclusive form reaches a `byte` upper bound without
arithmetic, which `byte` lacks ([§1](#primitive-types)):

```rs
let kind: byte = switch (c) {
    'a'..='z' => 'l',
    'A'..='Z' => 'u',
    '0'..='9' => 'd',
    else => '?',
};
```

> **Guarantee** — arms are checked in order, and no fallthrough occurs — exactly
> one arm's expression is evaluated.

`switch` is an expression, with the same single-expression/`yield` arm rules as
`match` ([§7.4](#pattern-matching)).

---

<a id="if-else"></a>
## 9. If / Else

```rs
if (condition) DoThing();
if (condition) DoThing(); else DoOtherThing();

if (condition) {
    ...
} else if (otherCondition) {
    ...
} else {
    ...
}
```

> **Constraint** — the condition must be `bool`; no implicit conversion from any
> other type.

> **Constraint** — braces are optional. Without braces, exactly one statement
> follows a branch — a sequence of statements requires `{ }`.

`if` is an expression. A single-statement branch's value is that statement's
expression; a block-bodied branch produces its value via an explicit
`yield expr;`, the same as `match`/`switch`
([§7.4](#pattern-matching)/[§8](#switch)).

> **Constraint** — when used as an expression, a final `else` is mandatory, and
> every branch must produce a value of the same type — there is no default value
> for an uncovered path.

```rs
let x: i32 = if (cond) 1 else 2;

let y: i32 = if (cond) {
    yield 1;
} else {
    yield 2;
};
```

This single-line form also serves as the ternary — no separate `? :` operator
exists.

> **Footgun: dangling else** — with braces optional, a bare `else` following
> nested unbraced `if`s attaches to the nearest preceding `if` that doesn't
> already have one — the same rule C uses. This is deterministic to the
> compiler, but can visually mislead a reader:

```rs
if (a)
    if (b)
        DoThing();
    else
        DoOther(); // binds to `if (b)`, not `if (a)`
```

Use braces when nesting to make the attachment visually unambiguous.

---

<a id="while"></a>
## 10. While

```rs
while (condition) DoThing();

while (condition) {
    ...
}
```

> **Constraint** — the condition must be `bool`; no implicit conversion from any
> other type.

> **Constraint** — braces are optional. Without braces, exactly one statement
> follows.

`while` is a statement, not an expression — its body may execute zero times, so
no value can be guaranteed to come out of it.

`break` exits the loop immediately; `continue` skips to the next condition
check.

> **Constraint** — both are valid only inside a loop body; using either outside
> one is a compile error.

> **Guarantee** — `break`/`continue` always affect the innermost enclosing loop —
> there is no labeled break/continue.

An infinite loop is written `while (true) { ... }`; there is no separate `loop`
keyword.

`break` and `continue` may appear inside a block-bodied arm of a `match`,
`switch`, or `if` nested in a loop. They are bound by the innermost enclosing
loop, not by the arm.

> **Guarantee** — `yield` always produces a value for the innermost enclosing
> expression — the `match`, `switch`, or `if` it appears in — never for the loop,
> which is a statement ([§10](#while), [§11](#for)). A yielded value belongs to
> that construct regardless of what control does next.

`break` and `continue` accept an optional `yield expr`, which produces a value
exactly as a bare `yield` does and then transfers control:

```rs
break yield x;    // the enclosing match/switch/if produces x; the loop exits
continue yield x; // it produces x; the iteration is abandoned
```

`x` is subject to the same type requirement as any other arm value
([§7.4](#pattern-matching), [§8](#switch), [§9](#if-else)). `continue yield` is
legal but never useful — the value it produces is discarded when the iteration
is abandoned — and is reported only by the ordinary unused-value diagnostics, if
any.

> **Constraint** — `break yield` and `continue yield` are valid only where the
> innermost enclosing expression construct lies inside the loop being left —
> there must be something for the value to be produced for.

> **Constraint** — if an arm is one that must produce a value — because its
> enclosing `match`, `switch`, or `if` is used as an expression
> ([§7.4](#pattern-matching), [§8](#switch), [§9](#if-else)) — reaching a bare
> `break` or `continue`, with no `yield`, is a compile error: that path produces
> no value, and the language has no default value for an uncovered path. Use
> `break yield <expr>;` to supply one. When the construct's value is discarded,
> its arms need no `yield` ([§7.4](#pattern-matching)) and bare
> `break`/`continue` are unrestricted.

---

<a id="for"></a>
## 11. For

```rs
for (i in 0..4) DoThing();    // i is 0, 1, 2, 3
for (i in 0..=3) DoThing();   // same sequence — 0..=3 is inclusive
for (b in s) { ... }         // each unit of slice s, in order
for (v in a) { ... }         // each element of fixed-size array a
```

`for` is a statement, not an expression — the iterated sequence may be empty, so
no value can be guaranteed to come out of it. The same reasoning applies to
`while` ([§10](#while)).

The iterated sequence is a range, a slice, or a fixed-size array. Iterating a
range binds the loop variable to each integer from `a` to `b` — inclusive of `b`
when written `a..=b`, exclusive of `b` when written `a..b`. Iterating a slice or
array binds it to each element, in order from the first to the last.

> **Constraint** — a range is not a value. `a..b` and `a..=b` may appear in
> exactly three positions: as a `switch` case ([§8](#switch)), after `in` in a
> `for`, and as a subscript ([§4.3](#sub-slicing)). A range cannot be bound,
> stored in a variable, passed to a function, or returned — there is no range
> type.

The loop variable is a per-iteration binding, not a reference, and is immutable
regardless of the iterated value's own binding.

> **Constraint** — reassigning it inside the body is a compile error; mutating an
> element reached through it requires writing to the collection by index or
> through a writable slice ([§4.2](#slices)).

`for` is sugar for an indexed `while`: stepping the index, comparing it against
the end, and reading the current element. `break`, `continue`, nesting, and
outermost-loop binding are therefore exactly [§10](#while)'s, with nothing
`for`-specific to define; `break` exits the `for`, and `continue` advances the
index past the current iteration.

---

<a id="strings"></a>
## 12. Strings

```rs
let s: byte[] = "hello";
```

A string literal is a `byte[]` slice ([§4.2](#slices)) viewing bytes placed in
`.rodata`. It never allocates, and there is no owning or growable string type —
an owning string, if one exists, is layered on the heap array in a later phase.

A string literal stores exactly its contents — there is no trailing null
terminator; the length is carried in the slice.

String literals support the same escape sequences as `byte` literals
([§1](#primitive-types)): `\0`, `\n`, `\r`, `\t`, `\\`, `\"`, `\'`, and `\xNN`
for a single byte by two-digit hex value. There is no raw-literal form.

> **Footgun** — a string literal must be valid UTF-8, but nothing at runtime
> checks or records this, and the type system does not distinguish encoded text
> from opaque bytes. It is an assumption the language is built on, not a property
> the compiler enforces for composed values.

> **Guarantee** — a string literal always decays to a read-only `byte[]` view. It
> is never `&mut`-taken, so it can never yield a writable view, and writing
> through it is a compile error — a consequence of the slice type's read-only-ness
> ([§4.2](#slices)), not a rule special to strings.

The built-in operations on a string are exactly [§4](#arrays-and-slices)'s slice
operations and nothing more: `.size` gives its length in bytes (not characters),
`s[i]` gives the `byte` at position `i`, and `s[a..b]` gives a `byte[]`
sub-slice. There is no
concatenation operator, no built-in formatting, and no built-in search —
producing text asks the allocator for space, which is a later phase's concern.

> **Footgun: text is bytes, not characters** — because a string is a `byte[]`,
> `.size`, indexing, and sub-slicing all count bytes. On multibyte UTF-8 text
> these do not align with characters: `"é".size` is 2, and `s[1..2]` may split a
> character mid-sequence, producing a slice that is no longer valid UTF-8. The
> language provides no protection against this; safe traversal requires the
> decoding helpers a later phase will add, and until then string handling is
> ASCII-correct only.
