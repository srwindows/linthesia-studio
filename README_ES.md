# Linthesia - Studio — 1.0.0 (primera versión comunitaria)

**Repositorio del fork:** https://github.com/srwindows/linthesia-studio

## Proyecto comunitario de software libre

**Cualquiera puede utilizar, estudiar, modificar y compartir Linthesia - Studio y aportar mejoras bajo la GPL.** Se aceptan propuestas en GitHub Issues y contribuciones mediante Pull Requests; las revisiones dependen de colaboradores voluntarios, sin obligación de mantenimiento permanente por parte de ninguna persona. También buscamos voluntarios para el empaquetado Debian.

No se declara el código de terceros de **dominio público**: se respetan la licencia GPL y los derechos y créditos de Linthesia y sus autores originales.

Consulta [cómo colaborar](CONTRIBUTING.md) y los [principios de la comunidad](COMMUNITY.md).

[Historial y notas de la versión 1.0.0](CHANGELOG.md).

**Origen del proyecto:** [linthesia/linthesia](https://github.com/linthesia/linthesia).
Linthesia es software libre derivado de trabajo previo; **no se presenta como una aplicación creada desde cero**. Se conservan los derechos y créditos de los autores originales, incluido Nicholas Piegdon y la adaptación de Oscar Aceña, y la licencia incluida en [`COPYING`](COPYING) (GNU GPL v2, conforme a los avisos del código original). Los cambios añadidos deben mantener los avisos y licencias correspondientes. No se incorpora código propietario actual de Synthesia ni SeeMusic.

## Estado de esta revisión

Este árbol es una **beta de desarrollo**, no una versión final certificada. Entre las funciones implementadas y verificadas se encuentran: reproducción MIDI con FluidSynth y SoundFonts SF2/SF3, elección de instrumento por pista, práctica de manos izquierda/derecha/ambas, teclado de 88 teclas, pentagrama, controles de tempo, modo 2D y escenarios Blender 3D, notas fluorescentes con animación de contacto, y exportación de vídeo (MP4, MKV, MOV, WebM y GIF) con carpeta de destino elegida por el usuario.

La detección musical contextual actual es un **motor de heurísticas y reglas**, no una red neuronal entrenada ni un arreglista automático infalible. Las correcciones se ofrecen para aprobación y se guardan sin sobrescribir el MIDI original.

## Debian 13: desarrollo y ejecución

Requiere compilador C++14, Meson, Ninja y las bibliotecas declaradas en `BUILD-DEPENDS` y `meson.build`. Para audio se utilizan FluidSynth y el sistema de sonido disponible; la exportación exige `ffmpeg`. El selector gráfico de destino utiliza `kdialog` cuando está instalado.

```bash
meson setup build --prefix="$HOME/.local/opt/linthesia"
meson compile -C build
meson test -C build --print-errorlogs
meson install -C build
```

La aplicación se abre desde el lanzador instalado o el binario correspondiente. Los archivos MIDI y el banco SF2 son externos y configurables.

## Exportar vídeo a una ubicación elegida

Desde la pantalla de pistas pulsa **Exportar vídeo del escenario**. Alternativamente, durante la reproducción abre **Ajustes → Exportar vídeo**. En la ventana resultante elige **Elegir carpeta** o **Guardar como…** y después **Generar vídeo**. La última carpeta se recuerda para usos posteriores. La ubicación real aparece antes de comenzar y el programa informa del resultado al terminar.

## Límites conocidos antes de publicar

- La calidad visual del escenario, los personajes y los efectos está en evolución; aprobar pruebas técnicas no demuestra equivalencia artística con SeeMusic.
- El análisis contextual de manos y notas no equivale a una IA musical entrenada; cada sugerencia necesita revisión del músico.
- La reproducción de pistas que comparten canal MIDI puede limitar el cambio realmente independiente de instrumentos en SoundFonts; requiere validación con MIDIs multicanal complejos.
- La exportación con audio depende de que la salida del sistema pueda monitorizarse; un instrumento MIDI físico puede producir sonido fuera de esa ruta.
- No incluir en GitHub carpetas `analysis/`, `auditorias/`, `backups/`, archivos personales, modelos o informes extraídos de software propietario ni datos de configuración de usuario.

## Validación

```bash
meson test -C build --print-errorlogs --num-processes 4
```

Las pruebas incluyen comprobaciones de OpenGL real con Mesa, Audio/FluidSynth, detección de manos, MIDI, interfaz y exportación. Las pruebas funcionales deben repetirse en una instalación limpia antes de etiquetar una release pública.
