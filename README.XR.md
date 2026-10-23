# FLTube for DomeD

Upstream: https://gitlab.com/facuA/fltube, revision
`4313faf925c5784e640fc3ba74398489b8b01573` (version 2.2.0).
The complete upstream Git history and GPL-3.0 license are retained.

FLTube searches YouTube through yt-dlp and launches **mpvr** by default. The
original webpage URL goes straight to mpv's yt-dlp hook, keeping adaptive audio,
headers, seeking, titles and available stereo metadata. Playback uses an argument
vector rather than a shell pipeline; quotes, Unicode and URL metacharacters are
preserved. Non-mpvr players remain available through `STREAM_PLAYER_PATH` and
retain upstream's streaming behavior.

Choose **XR video → Automatic, Full SBS, Half SBS, Full top-bottom, Half top-bottom,
or Mono** before opening a video. Automatic uses available source metadata and
explicit packing markers in the resolved title. An unlabelled “3D” video requires
a manual choice. Layout selection persists between launches. Full means each
stored eye has its normal dimensions; half means the packing axis is squeezed.
Use `--swap-eyes` in `STREAM_PLAYER_PARAMS` for reversed source eyes.

An active DomeD XR session routes playback to mpvr's dedicated stereo surface.
Inactive sessions use normal desktop playback. No session or display manager is
started or restarted. mpvr repacks top/bottom to its established SBS transport on
the GPU. This handles existing flat stereoscopic video; it does not synthesize
depth or project spherical VR180/360, anaglyph or checkerboard video.

## Command-line reference

Run `fltube --help` or read [fltube(1)](docs/man/fltube.1) with
`man -l docs/man/fltube.1`. `fltube --config=/absolute/path/fltube.conf` selects
an existing configuration file; the equals sign is required. Video URLs are
entered in the window rather than passed as positional arguments. `--debug`
and `--version` are also supported. The source manpage is not yet part of the
CMake install manifest.

## Desktop conventions

The FLTK menu exports real commands through DBusMenu and the native Wayland
AppMenu protocol (X11 properties as fallback). File, Options, XR video and Help
appear in DomeD's universal menu; its standard Edit and Window commands remain
available. Existing FLUID submenu pointers stay intact, including callbacks,
visibility, disabled state, quality radio buttons and cache/history controls.

DomeD's palette, accent, bitmap font and size follow
`$XDG_CONFIG_HOME/dome/appearance.json` live through the same MIT-licensed bridge
used by sibling de-xr apps. Local color overrides are disabled in Dome sessions.
Other desktops retain FLTube's themes. Settings, data and cache honor XDG paths.

Ctrl+L focuses search/URL input; Ctrl+Q quits. Existing configurable shortcuts for
search, results, channels and help remain supported. No Super shortcuts are
captured. The player retains mpv's Space, arrows and Q playback controls and its
own native universal-menu export.

## Optimized build and staged installation

Dependencies: patched FLTK 1.4.5, CMake, C++17 compiler, curl, Fontconfig,
nlohmann-json, GIO, zlib, libwebp, OpenGL, Wayland/X11 development headers,
wayland-scanner and gettext.
Runtime playback needs the updated mpvr and yt-dlp.

```sh
cmake -S . -B build-release -G Ninja \
  -DCMAKE_PREFIX_PATH="${FLTK_PREFIX:-/opt/fltk-vr/toolkit}" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/opt/fltube"
cmake --build build-release --parallel 6
ctest --test-dir build-release --output-on-failure
DESTDIR="$PWD/stage" cmake --install build-release --strip
ldd "stage$HOME/.local/opt/fltube/bin/fltube"
cmake --install build-release --strip
```

The binary's runtime search path selects the patched FLTK. The selected prefix
also controls resource, translation, icon and default configuration paths.
The Makefile is a CMake convenience wrapper; `PREFIX` is the install destination
and `DESTDIR` is the staging root. Use `FLTK_PREFIX` to select another toolkit.
System installation requires sudo only for the final install, after verification.

## Validation

The following describes the repository's existing validation harnesses and
previously recorded results, not a fresh headset or live-network certification.


`ctest` checks the C++ player launch path, exact URL arguments, quoted options,
layout, custom yt-dlp path, exit status and rejection of shell command evaluation.

`/usr/bin/python3 tests/test_domed_ui.py` uses a private D-Bus, Xvfb and nested
DomeD release binary. It checks the native menu, live radio state, existing
Options callbacks, disabled theme controls, live light/dark appearance, saved
layout and clean exit. Screenshots and reports are under ignored `artifacts/`.
It needs Python GI, Xlib and Pillow. It does not modify the active session.

The companion mpvr suite covers rendered stereo eyes, aspect fitting, subtitles,
audio, seeking, controlled HTTP/yt-dlp resolution and nested DomeD XR delivery.
A live YouTube check also decoded three seconds of video/audio successfully.
Through-headset perception and sustained hardware performance remain untested.

## System-wide installation

For a system installation, configure `CMAKE_INSTALL_PREFIX=/usr/local` and
`CMAKE_PREFIX_PATH=/opt/fltk-vr/toolkit`, build as a regular user, and verify a
`DESTDIR` stage before running `sudo cmake --install build-release --strip`.
The binary uses the installed patched toolkit without depending on a developer's
home directory. Resources are installed under `/usr/local/share/fltube`.

Install the matching mpvr launcher, native player, Lua script and shader system
wide as well. Reopen running players to use an updated binary. Desktop and display
manager sessions do not need to be restarted.

To validate the system frontend, run
`FLTUBE_BINARY=/usr/local/bin/fltube /usr/bin/python3 tests/test_domed_ui.py`.

## Menu, font and thumbnail corrections (2026-10-23)

The local menu leaves the window only after a real universal-menu registrar and
window association are available; it returns when the host disappears. The
external FLUID submenu fingerprint remains intact, so state changes still reach
the universal menu. The appearance bridge resolves Dome's qualified BDF font
names and integer scaling, including bundled fonts, instead of falling back to
Helvetica. Ctrl+L in the search field selects its text without inserting a control
character.

Thumbnail URLs are used as returned by yt-dlp, with a primary/last-thumbnail
fallback. Download HTTP failures are rejected. JPEG/PNG/WebP decoding follows
file contents, validates dimensions and fits both row dimensions. Failed decodes
replace old content with a placeholder. The FLTK OpenGL widget driver does not
implement raster drawing, so the port supplies a GL image adapter for thumbnails
and PNG action icons while retaining CPU drawing for image surfaces.

`ctest` covers real image decoding, corrupt responses, aspect fitting, GL widget
pixels and player argument handling. The private DomeD menu test also accepts
`FLTUBE_TEST_FONT` and `FLTUBE_TEST_FONT_SIZE` for a selected BDF face; the
Galmuri11 qualified face at 17 pixels was checked. A live YouTube thumbnail
response was WebP despite its JPEG URL and decoded correctly in the new loader.
