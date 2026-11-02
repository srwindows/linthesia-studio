# Linthesia — piano de cola 3D y cámara orbital (22-10-2026)

## Implementado en C++ original y Blender

Las siete escenas están almacenadas en `Linthesia_Pack_7_Escenarios.blend` (Blender 4.3.2). El antiguo modelo rectangular de 4,85 m se sustituyó por un instrumento de cola modelado con un contorno curvo de aproximadamente 1,68 m de anchura máxima y 2,18 m de fondo, y un teclado de 52 teclas blancas y 36 negras, de A0 a C8 más marco. Hay tapa levantada, arpa/cuerdas, tres patas, pedales y banqueta. El pianista permanece sentado con muñecas dentro del rango de teclas y once poses.

El nuevo motor `src/LiveStage3D.cpp` carga directamente 17 mallas de Blender (`graphics/scenarios/*.l3d`), compilándolas en listas OpenGL para evitar recompilar la geometría cada fotograma. Una cámara 3D con proyección de perspectiva real gira alrededor del escenario, y sigue siendo independiente de la velocidad/posición MIDI. El teclado interactivo inferior y las notas que caen continúan en el plano de práctica original.

## Cómo utilizarlo

1. Iniciar Linthesia, abrir una canción y escoger **3D** con estilo **Cinemático** (valor 2). Seleccionar un escenario en **Ajustes → Escenarios**.
2. **Mantener el botón derecho y arrastrar** sobre el escenario: rotación horizontal y elevación.
3. **Rueda del ratón** sobre el escenario: zoom 3D real.
4. **Ajustes → Escenarios → Siguiente**: selector **Vista 3D** (Libre / Frontal / Izquierda / Derecha / Detrás / Cenital / Diagonal), y reguladores de giro, elevación y distancia.
5. **Guardar** conserva el escenario y la cámara. **Cancelar** recupera lo anterior. Las rotaciones y el zoom hechos fuera de la ventana de ajustes se guardan al terminar el gesto.

La cámara se guarda bajo `[Escenarios3D]` en `~/.config/linthesia/ajustes.ini` mediante `camara-giro`, `camara-elevacion`, `camara-distancia` y `camara-vista`. La vista diagonal predeterminada tiene giro de 26°, elevación de 29° y distancia de 450 cm. El teclado MIDI FANTOM-08 no se ha tocado en las pruebas.

## Blender: regeneración segura

- `assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend` contiene las siete escenas editables.
- Para regenerar la geometría de la cola sin borrar las seis escenas: `blender --background --python assets/scenarios/blender/generar_7_escenarios.py` (el script utiliza `grand_3d_export.py`). Este generador reconstruye el piano de cola y puede sobrescribir modificaciones manuales hechas directamente a ese modelo; haga una copia del `.blend` antes de ejecutarlo.
- Regenerar imágenes y poses prerenderizadas opcionales: `blender --background --python assets/scenarios/blender/render_pack_grand.py`. Estos TGA también deben actualizarse en `graphics/scenarios/`.
- La versión obsoleta del generador rectangular está archivada como `generar_7_escenarios_LEGACY_20261022.py`; no utilizarla para crear versiones actuales.

## Pruebas verificadas

- 68/68 pruebas Meson, incluyendo formato y existencia de las 17 mallas trianguladas, selección y límites de cámara y reactividad del reproductor.
- Sesión Xvfb aislada y binario instalado: cámara inicial, rotación mediante botón derecho, zoom con rueda, cambio a vista frontal desde Ajustes y Cancelar; persistencia confirmada en INI. Los eventos MIDI son sintetizados en un archivo de prueba, con puertos desconectados.
- Respaldo previo a esta intervención: `<ruta-del-proyecto-Linthesia>/backups/cola-orbital-20261022-1226`.

### Limitación importante

Las mallas son 3D reales y la cámara gira de verdad, pero el pianista sigue siendo estilizado y solo tiene once poses de manos asociadas de forma aproximada a actividad MIDI. **Aún no tiene cinemática inversa para asignar cada dedo a cada nota MIDI ni calidad humana fotorrealista**. El roll de notas y el piano interactivo inferior no giran junto a la escena porque deben mantenerse legibles al practicar.

## 22-oct-2026: interpretación MIDI por dedos

Se añadió `src/PianoHands.h`, un animador determinista de diez dedos que recibe las notas realmente activas del piano (incluidas las notas tocadas físicamente cuando el programa las registra). Reparte acordes entre las manos y actualiza trayectorias y presiones mediante ataque/sostenido/liberación. El renderizador OpenGL (`src/LiveStage3D.cpp`) construye en tiempo real codos, antebrazos, muñecas y tres falanges visibles por dedo, y 88 teclas 3D con descenso físico de 10–14 mm.

El anterior ciclo de once posturas se mantiene exclusivamente como imagen de reserva; el escenario 3D activo utiliza la geometría real de mano y tecla. El control `Movimiento del pianista` modifica la velocidad de gestos y, a 0, inmoviliza al personaje sin desactivar las teclas MIDI. Se añadieron normas suaves de Blender al formato de malla L3DM v2, compatible con los archivos anteriores v1. Modelo de Blender editado con mallas de cabello, rostro, cuello, traje y proporciones más adultas.

Los MIDI estándar **no contienen asignación de dedo**: las diez posiciones se eligen mediante un algoritmo plausible, no por información musical auténtica de digitación. Al superar diez notas simultáneas, las 88 teclas responden, pero solo pueden representarse diez dedos. Sigue pendiente un modelo humano fotorrealista con texturas PBR, captura de movimiento y cinemática inversa de precisión.
