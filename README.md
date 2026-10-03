
# FeatherUI

> ![WARNING]
> Expect dragons and bugs. Tons of bugs. You have been warned.
>
> This is in very early stages of development and at the moment unusable for
> anything but reading the code.
>
> Don't expect much from it any time soon if ever. I consider it an experiment
> which may turn right or be abandoned tomorrow, depending how it goes and how
> I feel about it.


The general and still somewhat blurry aim is to create `HWND`-less and very
lightweight control toolkit for Windows with the following properties:

 * Support Windows 10 & 11 (and use their features; certainly don't expect any
   crazy compatibility effort for XP from me).

 * Thin wrapper of WIN32 top-level windows (`HWND`) with complete rendering via
   Direct2d and DirectWrite.

 * C implementation with C easy-to-understand interface to anyone with some
   Win32 API experience.

 * GDI almost completely abandoned and move everything to graphical card.

   (The only exception may be an ability to create D2D resources like e.g.
   bitmaps by converting them from their GDI equivalents; in other words as an
   input method only.)

 * Maybe eventually header-only C++ wrapper (to avoid any C++ name mangling/ABI
   compatibility hell).

 * Respect the system settings (light versus dark mode, accent colors).

 * If possible, a visual analogy with WinUI 3, but without all the bloat.
   (A minimal Hello World app in WinUI 3 can have about 40 - 50 MB, that's just
   insanity.)

 * Ability to import/use/build dialoges from some kind of reasonable text input
   (maybe JSON?), which might also be embeddable as standard resources in
   an `.exe` or `.dll` file.
