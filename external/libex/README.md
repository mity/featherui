
# `libex` Readme

Home: https://github.com/mity/libex


## Overview

This is collection of assorted C utilities which can roughly be seen as an
extension of standard library, with the following properties:

 * **Highly reusable**
 * **No surprise**
 * **Minimal inner dependencies** so that projects can copy into their source
   tree only the stuff they need, without too much headache.


## Usage

### As any other library:

 1. Instruct your compiler to search the include directory
    (e.g. via `-Iinclude`) and use `#include "ex/headername.h"` in your source
    file(s) as needed.

    Alternatively it's also possible to use (e.g. via `-Iinclude/ex`) and then
    use `#include "headername.h"` if you're not afraid of header filename
    collisions with your own project or another library.

 2. Link your program with `libex` library.


### Direct reuse of relevant sources:

If your project needs only very little of the `libex` functionality, you may
prefer to directly copy the relervant `libex` sources and headers to your build
tree.

Note this is explicitly permitted; even without any accompanying file(s) such
as this `READMER.md` or `LICENSE.md` as long as you keep intact the copyright
and licensing notes at the beginning of all such source files.

(For this very purpose we try quite hard to keep the module inter-dependencies
to the required  minimum.)


## Compatibility Note

Please note we do **not** provide any formal compatibility guaranteed, both on
the binary as well as source level, at least at the moment.

Of course we don't change any our interface willy-nilly unless there's strong
need for it, but we reserve any right to do so when such change is seemed
worth the pain caused to the users.


## Documentation

Documentation of all public types and functions is provided in comments
directly in the respective header files.


## Table of Contents

### Data Structures

 * **`ex/buffer.h`:** Simple grawable/shrinkable buffer implementation.

 * **`ex/rbtree.h`:** Intrusive red-black tree implementation.

 * **`ex/stack.h`:** Simple stack implementation (wrapper of `ex/buffer.h`).

### Hash Functions

 * **`ex/fnv1a_32.h`:** 32-bit Fowler–Noll–Vo hash hash (variant FNV1a).

 * **`ex/fnv1a_64.h`:** 64-bit Fowler–Noll–Vo hash hash (variant FNV1a).

 * **`ex/murmur3_32.h`:** 32-bit MurMur3 hash by Austin Appleby.

### Memory Allocators

 * **`ex/goodalloc.h`:** Heuristics for good buffer allocation sizes.

 * **`ex/malloca.h`:** `MALLOCA()` and `FREEA()` macros, which are portable
   equivalents of `_malloca()` and `_freea()` from Windows SDKs by Microsoft.

 * **`ex/memchunk.h`:** Specialized allocator for situations when a lot of
   (small) allocations is needed, and which are then eventually all freed at
   once (whole "chunk"). Provides smaller overhead than `malloc`.

### Miscellaneous

 * **`ex/crc32.h`:** CRC32.

 * **`ex/defs.h`:** Miscellaneous macros such as `MIN`, `MAX`, `ABS`,
   `SIZEOF_ARRAY`, `OFFSETOF`, `CONTAINEROF`, with typical C implementations
   of those macros.

 * **`ex/cmdline.h`:** Lightweight command line (`argc`, `argv`) parsing.

### Windows-specific Modules

 * **`ex/memstream.h`:** Simple read-only memory-backed implementation of the
   COM interface `IStream`.


## License

`libex` is covered with MIT license, see the file `LICENSE.md`.


## Reporting Bugs

If you encounter any bug, please be so kind and report it. Unheard bugs cannot
get fixed. You can submit bug reports here:

* http://github.com/mity/libex/issues
