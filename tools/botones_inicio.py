#!/usr/bin/env python3
"""Añade rótulos en español sin alterar los bordes del botón Linthesia."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[1]
button=Image.open(root/'graphics/title_Exit.tga').convert('RGBA')
font=ImageFont.truetype(str(root/'graphics/FreeSerif.ttf'),21)
for name,label in [('title_Canciones','Canciones'),
                   ('title_Configuracion','Configuración')]:
    im=button.copy()
    pix=im.load()
    # Interior uniforme original; mantener borde, sombra y transparencia.
    for y in range(12,39):
        background=pix[23,y]
        for x in range(27,163):
            pix[x,y]=background
    draw=ImageDraw.Draw(im)
    bounds=draw.textbbox((0,0),label,font=font)
    width=bounds[2]-bounds[0]
    x=88-width/2
    draw.text((x+1,14),label,font=font,fill=(49,49,49,255))
    draw.text((x,13),label,font=font,fill=(221,221,221,255))
    im.save(root/'graphics'/f'{name}.tga')
    print(name,im.size,im.getchannel('A').getbbox())
