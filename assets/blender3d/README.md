# Modelos originales de Blender para Linthesia

Se conservan los derechos originales del proyecto Linthesia (Nicholas Piegdon), su adaptación GNU/Linux (Oscar Aceña) y la licencia GPL incluida en `COPYING`.

- `Linthesia_Modelos_3D.blend`: fuente editable Blender 4.3.2 con teclas blancas, negras, notas cápsula y una colección con las 88 teclas del piano.
- `../../tools/blender_generate_models.py`: construcción de los modelos, materiales y exportación de las mallas trianguladas al motor C++.
- `../../src/BlenderMeshes.h`: geometría triangular y normales exportadas desde Blender. No editar manualmente: regenerar desde Blender.

Regenerar: `blender --background --python tools/blender_generate_models.py` desde el proyecto.

El modo 3D de Linthesia utiliza las mallas reales; el modo 2D mantiene su sistema original. La animación de pulsaciones procede del estado MIDI y no requiere abrir Blender durante la reproducción.

La integración es un primer motor geométrico con iluminación OpenGL. No equivale todavía a un renderizador físicamente basado con sombras dinámicas y PBR de Blender en tiempo real.
