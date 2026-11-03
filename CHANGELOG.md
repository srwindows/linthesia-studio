# Linthesia - Studio — Historial de versiones

## 1.0.0 — 3 de noviembre de 2026

Primera publicación comunitaria del fork **Linthesia - Studio**. La denominación 1.0.0 corresponde a la versión de distribución del proyecto; no significa que todos sus modos y escenarios sean definitivos.

### Funciones y correcciones incluidas

- Reproducción y práctica de archivos MIDI, teclado virtual de 88 teclas, selección de manos y modos de práctica.
- Motor de sonido FluidSynth, salida MIDI configurable, selección de SoundFonts e instrumentos, y correcciones de volumen PCM.
- Biblioteca, explorador y configuración persistente; mejoras de localización en español.
- Notas 3D, teclado y escenarios con modelos Blender editables, animaciones, efectos cinematográficos y controles de partículas.
- **Velocidad de efectos real:** afecta a la vida y movimiento de llama, rayos, partículas, humo y estelas sin modificar el tempo MIDI.
- **Paleta independiente:** cambiar el color de los efectos no modifica el color de las notas descendentes ni el teclado.
- Exportación de vídeo, con selección de carpeta de destino y manejo de errores del proceso FFmpeg.
- Recursos Blender editables, 27 generadores recuperados y 65 mallas 3D regenerables en entorno aislado.
- 151 pruebas automatizadas superadas en el código preparado para esta publicación, incluidas pruebas reales de OpenGL y FluidSynth.
- Documentación comunitaria para contribuciones, incidencias y adopción voluntaria del mantenimiento Debian.

### Límites conocidos y distribución

- Los efectos 3D y el realismo de personajes todavía admiten mejoras visuales; no se afirma paridad con SeeMusic.
- La detección y separación de manos utiliza métodos heurísticos, no un modelo de IA totalmente entrenado.
- La independencia por pista puede verse limitada cuando un archivo MIDI usa el mismo canal para varios instrumentos.
- Algunas rutas de exportación de audio dependen de los dispositivos de audio disponibles.
- Las distribuciones Debian oficiales no han aceptado aún este paquete. El `.deb` comunitario excluye los 172 archivos MIDI heredados que carecen de acreditación de derechos de redistribución. El repositorio de código conserva los archivos heredados existentes.
- En Debian, instalar juntos `linthesia-studio` y `linthesia-studio-data`, o usar un gestor que resuelva automáticamente la dependencia.

El proyecto procede del Linthesia original bajo su licencia GPL y conserva sus créditos. [Guía para colaborar](CONTRIBUTING.md).

## 0.8.0 Beta 1 — 2 de noviembre de 2026

Publicación de pruebas anterior al nombre y al conjunto definitivo de correcciones 1.0.0. Se conserva como versión histórica y no debe confundirse con esta edición.
