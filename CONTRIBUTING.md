# Contributing to Linthesia - Studio / Cómo colaborar

Linthesia - Studio is a **community-driven, free-software project**. Contributions are welcome from anyone; prior permission is **not required** to propose a change. There is **no promise of permanent availability, support or maintenance** from any particular contributor.

Linthesia - Studio es un **proyecto comunitario de software libre**. Cualquier persona puede proponer mejoras sin solicitar autorización previa. Ninguna persona está obligada a mantener el proyecto indefinidamente.

## Ways to help / Formas de colaborar

- **Report bugs / Comunicar errores:** use a GitHub Issue with steps to reproduce, Debian/Linux version, logs and relevant screenshots; remove personal paths, tokens and MIDI/private files.
- **Suggest changes / Proponer mejoras:** open an Issue explaining the need and intended user experience.
- **Submit code / Aportar código:** fork the repository, create a branch, implement the change and open a Pull Request describing what changed and how it was tested.
- **Documentation, translations and accessibility / Documentación, traducciones y accesibilidad:** small contributions are welcome.
- **Debian packaging / Paquetes Debian:** the `debian/main` branch contains packaging sources. Volunteers with Debian packaging experience are welcome; being a sponsor does **not** automatically mean agreeing to maintain the package. Any official Debian upload needs someone who voluntarily accepts the appropriate maintenance role.

## Before opening a pull request / Antes de enviar cambios

1. Keep changes focused. Preserve existing functionality and user settings; do not silently rename internal paths or alter the project's MIDI files.
2. Build and test where possible:

   ```sh
   meson setup build --prefix=/usr/local
   meson compile -C build
   meson test -C build --print-errorlogs
   ```

3. Explain the change, compatible environments, and limitations. Screenshots are useful for interface or 3D changes; do not describe untested features as working.
4. **Respect the existing GPL licence and the rights of upstream contributors.** Keep original copyright notices. New contributions should be compatible with the GPL and clearly identify their authors and any other licences involved.
5. Do **not** upload copyrighted third-party MIDI files, proprietary shaders, extracted assets or code from closed-source apps, proprietary firmware, credentials, personal audio recordings or files whose redistribution rights are unclear. Link to public examples where appropriate instead.

## Decisions and maintainers / Decisiones y mantenimiento

Discussion in Issues and Pull Requests is encouraged. Technical decisions are made through review by available volunteers with repository permissions. There is **no guaranteed review deadline**, and submitting a contribution does not guarantee its inclusion. Anyone is free to continue the development under the GPL, including by publishing an independent fork.

Se valoran las propuestas en Issues y Pull Requests. Las revisiones dependen de las personas voluntarias disponibles y no tienen plazo garantizado. Cualquiera puede continuar el desarrollo mediante su propio fork respetando la GPL.

The original Linthesia project and authors remain credited in [README_ES.md](README_ES.md) and [COPYING](COPYING). The code **is not being relicensed into the public domain**: the GPL already guarantees use, study, modification and redistribution, subject to its terms, including commercial uses.
