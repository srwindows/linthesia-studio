# Procedencia y material fuente de los escenarios de Linthesia - Studio

El proyecto está derivado de Linthesia, bajo GPL-2.0-or-later. Los recursos 3D desarrollados para este fork son modelos y fondos editables; no proceden del código ni de los shaders propietarios de Synthesia o SeeMusic.

## Archivos modificables conservados

- `assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend`: proyecto Blender 4.3 con siete escenas: Pianista animado, Guitarrista, Baterista, Cantante, Cantante femenina, Dúo vocal y Banda completa. Se comprobó con Blender que contiene 1.034 objetos tipo MESH, materiales, cámaras y geometría modificable.
- `assets/scenarios/blender/Manos_Anatomicas_CC0_EDITABLE.blend`: modelo editable de manos y teclado.
- `assets/blender3d/Linthesia_Modelos_3D.blend`: modelo editable adicional.
- `assets/scenarios/blender/*.py` y `assets/scenarios/blender/humano_cc0/*.py`: 27 scripts originales de generación, transformación, renderización y exportación, recuperados de la copia de desarrollo. Los scripts usan `LINTHESIA_ROOT` o detectan automáticamente la raíz del proyecto para no depender de una carpeta de usuario concreta. Algunos scripts reconstruyen el `.blend` y pueden borrar ediciones posteriores; ejecutar solo sobre copias de trabajo.
- `assets/scenarios/blender/humano_cc0/base.obj`: malla de origen MakeHuman CC0, incluida para poder modificar la anatomía.
- `assets/scenarios/blender/humano_cc0/textures/middleage_male_skin_diffuse.png`: textura de piel de MakeHuman; se conserva el original editable.
- `graphics/scenarios/`: 65 mallas compiladas `.l3d` y 19 texturas/fondos `.tga`. El formato binario `.l3d` no sustituye a los proyectos Blender y scripts anteriores como fuente preferente de modificación.

## Licencias por procedencia

El proyecto MakeHuman distingue el código (AGPL-3.0) de sus **recursos principales** (CC0-1.0). Se utilizan la malla base y la textura de piel, no código AGPL de MakeHuman. Documentación oficial:

- https://github.com/makehumancommunity/makehuman/blob/master/LICENSE.md
- https://www.makehumancommunity.org/about/license.html
- Texto CC0: `assets/scenarios/blender/humano_cc0/LICENSE.ASSETS.md`.

Los objetos no derivados de MakeHuman —piano, instrumentos de banda, tarimas, luces, etc.— proceden del trabajo Blender de Linthesia - Studio. **Antes de presentar el paquete en el archivo oficial de Debian, el titular debe confirmar por escrito la autoría/licencia de esos objetos y la ausencia de materiales de terceros no acreditados.** La existencia de los ficheros editables demuestra la disponibilidad de fuentes, no por sí sola la titularidad jurídica.

## Reproducibilidad y límites

**Verificación práctica (3 noviembre 2026, Blender 4.3.2):** se ejecutaron en una copia temporal los scripts `grand_3d_export.py`, `crear_cinco_escenarios.py` y `humano_cc0/exportar_palmas_uv.py`. Se regeneraron correctamente los **65 archivos `.l3d`**, con los mismos nombres que los distribuidos. Dos mallas de las palmas CC0 coinciden byte a byte; los 63 archivos restantes difieren en su contenido, por lo que **no se afirma reproducibilidad bit a bit** ni equivalencia exacta del render. Los 19 fondos y texturas `.tga` necesitan verificación de regeneración independiente. Los originales de Linthesia no se modificaron durante la prueba.

Los archivos MIDI del repositorio upstream no son prueba de permiso sobre las composiciones musicales: se excluyen de la fuente Debian repaquetada mientras no se verifiquen sus licencias.

Los archivos `graphics/FreeSans.ttf` y `graphics/FreeSerif.ttf` proceden de GNU FreeFont (GPL con excepción para fuentes); el paquete Debian los sustituye por el paquete del sistema `fonts-freefont-ttf`. `graphics/InterfaceButtons/OpenSansEmoji.ttf` deriva de OpenSansEmoji (licencia Apache-2.0, comprobable mediante la metainformación de la fuente). Los demás recursos de interfaz originales requieren que se conserven los créditos y avisos de Linthesia GPL.
