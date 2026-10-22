# FLTube XR

FLTK YouTube browser for [DomeD](https://github.com/jebot-git/DomeD), with existing
stereoscopic video playback through [mpvr](https://github.com/jebot-git/mpvr).
Part of the [de-xr desktop](https://github.com/topics/de-xr).

- Search YouTube, browse saved lists and play the original URL through mpv's yt-dlp hook.
- Select automatic, full/half SBS, full/half top-bottom, or mono playback.
- Use DomeD's native universal menu, live appearance settings and conventional Ctrl shortcuts.
- Keep decoding, adaptive audio, synchronization and seeking in mpv.

This fork retains FLTube's complete upstream history and GPL-3.0 license.
Upstream: [GitLab](https://gitlab.com/facuA/fltube),
[GitHub mirror](https://github.com/FacundoAdorno/FLTube).
See the [original README](README_UPSTREAM.md).

## Run and configure

Start `fltube`, then enter a search term or a video URL in its input field.
There is no positional command-line URL argument. The supported options are:

```sh
fltube --help
fltube --version
fltube --debug
fltube --config=/absolute/path/to/fltube.conf
```

`--config` requires `=`, and the file must already exist. Otherwise FLTube reads
`$XDG_CONFIG_HOME/fltube/fltube.conf` (default `~/.config/fltube/fltube.conf`),
then the configuration under its compiled installation prefix, normally
`/usr/local/etc/fltube/fltube.conf`. Application settings, saved user data and
cache use the corresponding XDG configuration, data and cache directories.
Set `STREAM_PLAYER_PATH`, `STREAM_PLAYER_PARAMS` and `YTDLP_PATH` in that file
when using a different player or yt-dlp executable.

The [fltube(1) source manpage](docs/man/fltube.1) covers options, paths and stereo
layouts. Read it locally with `man -l docs/man/fltube.1`; the current CMake install
rules do not install this new page automatically.

## Build

Use the patched FLTK 1.4.5 from [fltk-vr](https://github.com/jebot-git/fltk-vr)
and an updated mpvr installation with top/bottom support.

```sh
cmake -S . -B build-release -G Ninja \
  -DCMAKE_PREFIX_PATH=/opt/fltk-vr/toolkit \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build-release --parallel 6
ctest --test-dir build-release --output-on-failure
DESTDIR="$PWD/stage" cmake --install build-release --strip
# Verify the staged files, then install system files:
sudo cmake --install build-release --strip
```

Launch `fltube`, then choose **XR video** before opening a video. Automatic
selection requires source metadata or explicit packing markers; choose a manual
layout for ambiguously labelled uploads. This player supports flat stereo video;
it does not synthesize depth or project spherical VR180/360 sources.

[Dependencies, playback, desktop integration and validation](README.XR.md).

The FLTK application windows use the shared DomeD OpenGL widget driver and
cached glyph textures. Image decode and network work remain on the CPU. Set
`DE_XR_GL_TRACE=1` to report the selected GL renderer for diagnostics.
