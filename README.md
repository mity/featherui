
<div align="center"><a href="https://github.com/mity/featherui"><img src="logo.webp" alt=FeatherUI></a></div>


> [!WARNING]
> Expect dragons and bugs. Tons of bugs. You have been warned.
>
> This is in very early stages of development and at the moment unusable for
> anything but reading the code.


## The Mission

The general and still somewhat blurry aim is to create `HWND`-less and very
lightweight GUI control toolkit for Windows with the following properties:

 * If possible, a visual analogy with WinUI 3, but without all the bloat.
   (A minimal Hello World app in WinUI 3 can have about 40 - 50 MB, that's just
   insanity.)

 * Support Windows 10 & 11 (and use their features). Older OS versions likely
   won't be supported.

 * Thin wrapper of WIN32 top-level windows (`HWND`) with complete rendering via
   Direct2D and DirectWrite.

 * C implementation with easy-to-understand interface to anyone with some
   Win32 API experience.

 * GDI completely abandoned; instead do all the graphics and text rendering
   via Direct2D and DirectText technologies.

   The only exception may be an ability to create D2D resources like e.g.
   bitmaps by converting them from their GDI equivalents; in other words just
   as (one of) input methods.

 * Respect the system settings (light versus dark mode, accent colors,
   animations etc.).

 * Maybe eventually header-only C++ wrapper (to avoid any C++ name mangling/ABI
   compatibility hell).

 * Ability to import/use/build dialogs from some kind of reasonable text input
   (maybe JSON?), which could also be linked-in as standard resources in
   `.exe` or `.dll` files.

 * Eventually also some GUI app for designing such dialog templates.


## Specific Non-Features

Or, in other words, what **not** to expect from this:

 * This is not intended as a multi-platform toolkit like Qt or Gtk, which
   create their own abstraction of (almost) complete system interface.


## Licensing

TL;DR: `SPDX-License-Identifier: GPL-2.0-or-later OR LicenseRef-Commercial`

This project is available under either of the following licensing options:

 * **GPL-2.0-or-later:**
   You may use, modify, and redistribute this project under the terms of
   the GNU General Public License, version 2 or (at your option) any later
   version, as described in [`LICENSE.gpl.md`](LICENSE.gpl.md).

 * **Commercial license:**
   If you wish to use this project under commercial licensing terms instead
   of the GPL, please refer to [`LICENSE.commercial.md`](LICENSE.commercial.md)
   for the applicable terms and conditions.

Contributions from others are accepted under the terms described in the file
[`CONTRIBUTING.md`](CONTRIBUTING.md). These terms ensure that the project can
continue to be offered under both licensing options.

For the full terms and conditions, please read the relevant license document.

### Exceptions

Code in the `demo/` and `examples/` directories is intended to serve as
reusable examples and may be used, copied, modified, and distributed without
restriction, including for commercial purposes.

Third-party code in the `external/` directory is licensed under the MIT
License. The MIT License is compatible with this project's dual-licensing
model, and this code may be distributed as part of the project under the
applicable project licensing terms, subject to the MIT License's requirements.
See the relevant copyright and license notices for details.
