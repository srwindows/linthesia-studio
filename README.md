# Linthesia - Studio

**Linthesia - Studio 1.0.0 — primera versión comunitaria.** Maintained derivative work: this repository builds on the upstream [Linthesia](https://github.com/linthesia/linthesia) GPL-licensed project, retaining its history, original authors and licensing. It adds modern MIDI/FluidSynth, practice, 3D and video-export functionality. [Documentación en español y limitaciones conocidas](README_ES.md). The software is under active development and is not a product of the current proprietary Synthesia or SeeMusic projects.

**GitHub:** https://github.com/srwindows/linthesia-studio

## Community project / Proyecto comunitario

**Free to use, study, modify, share and contribute to under the GPL.** Anyone can propose improvements through Issues and Pull Requests. The project welcomes voluntary maintainers, including volunteers for Debian packaging; no individual is committed to maintaining the software indefinitely. This is **GPL-licensed free software, not a public-domain dedication**, and third-party credits and licensing remain in force.

See [How to contribute](CONTRIBUTING.md) and [Community principles (Español)](COMMUNITY.md).

[Version history / Historial 1.0.0](CHANGELOG.md).


Linthesia - Studio builds on the original open-source Linthesia project, itself a fork of the earlier Synthesia software. It is a game of playing music using a MIDI keyboard (or your PC keyboard), following a .mid file.

Synthesia up to version 0.6.1a is Open Source. This project uses the latest source from sourceforge.

## Build

To compile, you need a basic c++ toolchain, and satisfy all dependences which are on BUILD-DEPENDS file. Then, just:

    $ meson --prefix /usr build
    $ ninja -C build
    $ ninja -C build install #probably with sudo

## How to use
- In most case, just run `/usr/bin/linthesia` binary, and Enjoy :-)
- Linthesia provides you with several musics that will be installed in `/usr/share/linthesia/music/` you can use `--lib-path` to define an other path when required. (read `linthesia -h` since last used path is preserved).
- Linthesia will use anything it finds to play sound. You might want to install `fluidsynth` or `TiMidity++` to run the midi server required for it to produce a sound. You keyboard might provide output that can (probably) be used too, if wanted.

## Credits

Visit https://github.com/linthesia/linthesia for more info.

Join the chat at https://gitter.im/linthesia/linthesia
