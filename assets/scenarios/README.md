# Linthesia — Pack de escenarios 3D v1

**Proyecto Blender original:** `assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend`
**Programa:** `<carpeta-personal>/.local/opt/linthesia/bin/linthesia`

## Selección y configuración

En Linthesia, abre **Configuración → Escenarios 3D**. El selector muestra siete
escenarios y sus miniaturas, además de la opción **Sin escenario (fondo actual)**.
Las opciones se guardan al pulsar **Aplicar** o **Aceptar**.

También pueden cambiarse durante la reproducción desde
**Ajustes → Escenarios**, con vista previa antes de pulsar **Guardar**.
Controles disponibles: **Escenario**, **Potencia de luces** (0–100%),
**Color de luces** (7 familias), **Movimiento de cámara** (0–100%) y
**Movimiento del pianista** (0–100%). No modifican el MIDI ni sus
colores de pista.

Las opciones se guardan en `~/.config/linthesia/ajustes.ini`, grupo
`[Escenarios3D]`. Predeterminado `escenario=-1` para **no cambiar**
el fondo de ningún usuario que no seleccione expresamente un escenario.

## Escenarios incluidos

0. Minimalista — composición arquitectónica limpia
1. Concierto oscuro — focos y cerchas
2. Neón futurista — tubos verticales cromáticos
3. Auditorio clásico — madera y paneles cálidos
4. Galaxia — estrellas y profundidad
5. Fuego cinematográfico — brasas ambientales
6. Pianista animado — músico modelado y animado en Blender, luces y cámara

## Qué es realmente 3D en esta versión

Los siete escenarios fueron modelados **en Blender 4.3.2** como escenas
editables con geometría, materiales, cámaras y luces de Blender.
Para no alterar la cadencia de Linthesia, el reproductor combina
**fondos prerenderizados de Blender** (TGA 960×540), las cápsulas y piano
3D originales de Linthesia, proyección/recorte existentes, una parallax
horizontal suave y cuatro haces de luz **OpenGL animados**. Esta técnica
es composición 2.5D, **no importa/renderiza las mallas completas de Blender
en tiempo real**.

El pianista tiene cabeza, cuerpo, manos y dedos modelados como objetos
separados con keyframes editables; se generaron once fotogramas de una
animación que cambian durante la reproducción en función de la actividad
MIDI. El control de movimiento permite desactivarla.

**Limitaciones actuales:** es un personaje humano estilizado, NO
fotográficamente realista; su actuación responde al número/registro de
notas, NO es una animación cinemática inversa de cada dedo sobre cada nota
de la partitura. La cámara del reproductor simula un desplazamiento 2.5D,
mientras que el proyecto Blender sí dispone de cámara 3D real con
fotogramas clave. Una puesta en escena humana fotorrealista, con contacto
preciso de dedos y captura de movimiento, requerirá un personaje humano
riggeado de alta calidad y sincronización MIDI por mano/dedo.

## Edición en Blender y regeneración

```sh
blender <ruta-del-proyecto-Linthesia>/assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend
```

En Blender, cambiar la **escena activa** del desplegable superior; se
pueden editar libremente las geometrías y las luces denominadas
`LUZ EDITABLE`, así como la `CAMARA EDITABLE`. En el escenario
**Pianista animado**, inspeccionar los controladores de manos y el
Dope Sheet/Timeline.

El script fuente que genera el pack es
`assets/scenarios/blender/generar_7_escenarios.py`. Volver a ejecutarlo
**recrea todas las escenas desde cero**, por tanto no hacerlo después de
editar a mano el `.blend` sin guardar una copia. Para que las
modificaciones lleguen a Linthesia es preciso volver a renderizar y
convertir los PNG a `graphics/scenarios/*.tga`; luego reinstalar los
archivos. Un cambio en `.blend` no actualiza automáticamente las
texturas del reproductor.

Los recursos están en:
- `assets/scenarios/blender/escenario_00.png` … `escenario_06.png`
- `assets/scenarios/blender/pianista_07.png` … `pianista_61.png`
- `assets/scenarios/blender/vista_previa_siete_escenarios.png`
- `assets/scenarios/blender/pianista_animacion_demo.mp4`
- `graphics/scenarios/escena_00.tga` … `escena_06.tga`
- `graphics/scenarios/pianista_00.tga` … `pianista_10.tga`

Respaldo del proyecto antes de integrar escenas:
`<ruta-del-proyecto-Linthesia>/backups/scenarios-20261022/`

## Pruebas

64/64 pruebas automatizadas correctas (incluyendo `test_stage_scenes.cpp`
y persistencia/clamp de todos los ajustes). Además se verificaron
manualmente el selector con miniaturas, fondos detrás de notas MIDI,
escenario del pianista, luces editables y recuperación de la selección
tras reiniciar en una sesión gráfica aislada sin enviar MIDI al FANTOM.
