Project: c_d2d_dwrite
URL: https://github.com/mmozeiko/c_d2d_dwrite
License: Unlicense
Copyright: Public Domain
Upstream version: 383a7afe49c12f5687390b10299df036fbb47f9c

Motivation:
 * We use these instead of `<d2d1.h>` and `<dwrite.h>` when we're being built
   with MSVC as Microsoft SDK has those broken and unusable from the C language.

Usage:
 * An empty preprocessor macro `WIN_NOEXCEPT` must be defined before including
   the headers. (At least with msys2/ucrt64 toolchain.)

 * `<cd2d.h>` requires `<cdwrite.h>` to be included first (at least when
   building with gcc toolchain, as gcc needs to know the definition
   of `enum DWRITE_PAINT_FEATURE_LEVEL`.
