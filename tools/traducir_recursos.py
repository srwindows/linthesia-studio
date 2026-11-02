#!/usr/bin/env python3
"""Localización es_ES de las láminas TGA originales de Linthesia.
Reconstituye cada recurso desde origin/main para que sea reproducible.
No cambia la geometría, zonas clicables ni logotipo.
"""
import io
import subprocess
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont

ROOT=Path(__file__).resolve().parents[1]
FONT=ROOT/'graphics/FreeSerif.ttf'
SANS=ROOT/'graphics/FreeSans.ttf'

def source(name):
 b=subprocess.check_output(['git','-C',str(ROOT),'show',f'origin/main:graphics/{name}.tga'])
 return Image.open(io.BytesIO(b)).copy()

def font(sz, serif=True):
 return ImageFont.truetype(str(FONT if serif else SANS),sz)

def write(im,n):
 im.save(ROOT/'graphics'/(n+'.tga'),format='TGA')
 print('GRÁFICO ES:',n,im.size,im.mode)

def clean_line(im,rect,sample_x,method='row'):
 px=im.load()
 x0,y0,x1,y1=rect
 for y in range(y0,y1):
  bg=px[sample_x,y]
  for x in range(x0,x1):
   px[x,y]=bg

button_names={
 'title_Exit':'Salir',
 'title_ChooseTracks':'Elegir pistas',
 'title_BackBox':'Volver',
 'tracks_PlaySong':'Tocar canción',
 'tracks_BackToTitle':'Volver al inicio',
 'score_RetrySong':'Repetir canción',
}
for n,label in button_names.items():
 im=source(n).convert('RGBA')
 # Keep arrows at x<36 and x>110 for paging.
 bounds=(42,11,137,38) if n.startswith('title_ButtonPage') else (25,10,169,39)
 clean_line(im,bounds,21 if not n.startswith('title_ButtonPage') else 38)
 d=ImageDraw.Draw(im)
 f=font(17 if len(label)>14 else 21)
 bb=d.textbbox((0,0),label,font=f)
 w=bb[2]-bb[0]
 if w>148:
  f=font(16);bb=d.textbbox((0,0),label,font=f);w=bb[2]-bb[0]
 x=90-w/2; y=14
 d.text((x+1,y+1),label,font=f,fill=(32,32,32,255))
 d.text((x,y),label,font=f,fill=(220,220,220,255))
 write(im,n)

# Page controls use the same untouched beveled frame as the footer.
# Old bitmap labels were left under the overlay; reconstruct both cleanly.
for n,label,back in [('title_ButtonPageBack','Anterior',True),
                     ('title_ButtonPageNext','Siguiente',False)]:
 im=source('title_Exit').convert('RGBA')
 clean_line(im,(25,10,169,39),23)
 d=ImageDraw.Draw(im)
 f=font(19)
 bb=d.textbbox((0,0),label,font=f)
 x=88-(bb[2]-bb[0])/2
 d.text((x+1,15),label,font=f,fill=(32,32,32,255))
 d.text((x,14),label,font=f,fill=(220,220,220,255))
 if back:
  d.polygon([(20,24),(36,14),(36,34)],fill=(204,220,215,240))
 else:
  d.polygon([(153,14),(168,24),(153,34)],fill=(204,220,215,240))
 write(im,n)

box_names={
 'title_SongBox':'Canción:',
 'title_OutputBox':'Dispositivo de salida:',
 'title_InputBox':'Dispositivo de entrada:',
 'title_DirBox':'Carpeta:',
}
for n,label in box_names.items():
 im=source(n).convert('RGBA')
 clean_line(im,(13,6,290,39),335)
 d=ImageDraw.Draw(im)
 d.text((14,9),label,font=font(21),fill=(213,213,213,255),
        stroke_width=0)
 write(im,n)

# Static credits on the main screen, preserved with Spanish attribution.
n='title_GameMusicThemes'
im=Image.new('RGBA',source(n).size,(0,0,0,0));d=ImageDraw.Draw(im)
f=font(11,False)
d.text((0,1),'La música de videojuegos procede de Game Music Themes.',font=f,fill=(222,222,222))
d.text((0,20),'Otros MIDI proceden del proyecto Mutopia.',font=f,fill=(222,222,222))
d.text((0,40),'Más canciones: http://www.mutopiaproject.org',font=f,fill=(114,189,244))
write(im,n)

# Statistics panel, same sprite dimensions and native numerical alignment.
n='stats_text'
im=Image.new('RGB',source(n).size,(64,64,64));d=ImageDraw.Draw(im)
d.text((24,40),'Resultados',font=font(33,False),fill=(111,162,207))
for label,y,sz in [
 ('Calificación:',150,22),
 ('Puntuación:',230,21),
 ('Velocidad media:',264,19),
 ('Notas acertadas:',337,20),
 ('Notas falladas:',373,20),
 ('Combo máximo:',445,20),
]:
 d.text((24,y),label,font=font(sz,False),fill=(215,215,215))
write(im,n)

n='play_Status'
im=source(n).convert('RGB')
# The status pane is an RGB texture with gray base, text in the first 100px.
# Recreate from the background shade while keeping graphic layers elsewhere.
clean_line(im,(0,2,112,40),118)
clean_line(im,(0,42,66,63),79)
d=ImageDraw.Draw(im)
d.text((1,6),'Puntos:',font=font(22,False),fill=(220,220,220))
d.text((1,44),'Tiempo:',font=font(12,False),fill=(180,180,180))
write(im,n)

# Contextual keyboard help over the piano, originally hard-coded in English.
n='play_Keys'
im=source(n).convert('RGBA'); d=ImageDraw.Draw(im)
d.rectangle((80,0,256,127),fill=(0,0,0,0))
d.rectangle((331,0,511,127),fill=(0,0,0,0))
for y,left_label in [(1,'Cambiar zoom'),(33,'Cambiar velocidad'),
                     (65,'Cambiar octava'),(97,'Cambiar volumen')]:
 d.text((82,y),': '+left_label,font=font(16,False),fill=(199,199,199,255))
for y,right_label in [(1,'Pausa'),(33,'Volver al menú'),
                      (65,'Mover posición')]:
 d.text((334,y),': '+right_label,font=font(16,False),fill=(199,199,199,255))
# Recreate keycaps so no fragments of baked English names remain.
def keycap(box, lines, text_size):
 x0,y0,x1,y1=box
 d.rounded_rectangle(box,radius=4,fill=(205,205,205,255),
     outline=(156,156,156,255),width=1)
 f=font(text_size,False)
 if isinstance(lines,str): lines=[lines]
 total=len(lines)*(text_size+1)
 for j,line in enumerate(lines):
  bbox=d.textbbox((0,0),line,font=f)
  tw=bbox[2]-bbox[0]
  d.text(((x0+x1-tw)/2,y0+(y1-y0-total)/2+j*(text_size+1)-1),
         line,font=f,fill=(53,53,53,255))
keycap((260,4,323,30),'Espacio',10)
keycap((261,66,283,97),['Re','Pág'],8)
keycap((299,66,322,97),['Av','Pág'],8)
write(im,n)

n='trackbox'
im=source(n).convert('RGBA')
# labels appear in the static panel top left; leave panel icon/color unchanged.
clean_line(im,(16,10,130,63),145)
d=ImageDraw.Draw(im)
d.text((17,13),'Instrumento:',font=font(14),fill=(234,234,234,255))
d.text((17,40),'Notas:',font=font(14),fill=(234,234,234,255))
write(im,n)
print('RECURSOS TRADUCIDOS:',len(button_names)+len(box_names)+4)
