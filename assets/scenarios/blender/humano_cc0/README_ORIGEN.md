# Origen y licencia — geometría anatómica

La malla humana base procede de **MakeHuman Community**:

- Repositorio: https://github.com/makehumancommunity/makehuman
- Archivo fuente: `makehuman/data/3dobjs/base.obj`
- Licencia de los recursos: **Creative Commons CC0 1.0** (el texto de licencia se conserva en `LICENSE.ASSETS.md`)
- Titulares identificados en la malla: **Data Collection AB, Joel Palmius y Jonas Hauquier** (liberación explícita de los recursos bajo CC0 en septiembre de 2020)

El proyecto importa exclusivamente geometría CC0 (cabeza y torso), adaptada a la posición del pianista en Blender. **No incorpora código fuente de MakeHuman**, cuyo código tiene otra licencia. Las modificaciones y animaciones del reproductor pertenecen al código fuente de Linthesia y mantienen su licencia original.

Los scripts locales (`integrar_cabeza_anatomica.py`, `terminar_cabello_anatomico.py`, `torso_anatomico.py`, `terminar_ropa_pianista.py`) son procesos reproducibles de elaboración del modelo `.blend` final. El resultado no debe describirse como una captura de movimiento o un humano fotorrealista: la asignación de dedo se estima a partir del MIDI y el modelo usa geometría anatómica con iluminación OpenGL clásica.

## Piel fotográfica con UV (octubre de 2026)

Textura CC0 MakeHuman `middleage_lightskinned_male_diffuse.png`, 2048 × 2048, archivo local `textures/middleage_male_skin_diffuse.png`. Fuente pública original:
https://download.tuxfamily.org/makehuman/assets/1.1/base/skins/textures/middleage_lightskinned_male_diffuse.png

Se conservaron y asignaron las coordenadas UV de `base.obj` sin aproximaciones planares. El material utiliza el mapa fotográfico en Blender y el renderizador OpenGL lo consume como TGA propio `graphics/scenarios/piel_masculina_cc0.tga` bajo el formato de malla L3DM v3 (RGB, normales, UV y marcadores de malla móvil). La textura está también empaquetada dentro del `.blend` para que el archivo editable siga siendo autocontenido. El rostro sigue limitado por la iluminación OpenGL clásica y no debe describirse como fotorrealismo cinematográfico completo.
