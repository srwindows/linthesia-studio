"""Evita regresiones visuales de 3D en Linthesia (GPLv2)."""
from pathlib import Path
import sys

source=Path(sys.argv[1])
renderer=(source/'src/Renderer.cpp').read_text()
keyboard=(source/'src/KeyboardDisplay.cpp').read_text()
settings=(source/'src/AdvancedSettings.h').read_text()
player=(source/'src/PlayingState.cpp').read_text()
assert 'k=std::min(k,0.42f/height);' in renderer
assert 'const float C=(bottomY-horizonY)*(1.0f+k*height)/height;' in renderer
assert 'bottomY+(bottomY*k-C)*bottomY' in renderer
assert 'void Renderer::BeginPianoRollClip(' in renderer
assert 'void Renderer::EndPianoRollClip(' in renderer
assert 'renderer.BeginPianoRollClip(x,sceneTop,x+m_width,y+y_offset);' in keyboard
assert 'renderer.EndPianoRollClip();' in keyboard
assert 'player3DDepth=38,player3DAngle=20,player3DHorizon=10' in settings
assert 'void Apply3DProfile(' in player
assert 'prefs.player3DDepth=46;' in player
assert 'prefs.player3DDepth=38;' in player

# La homografía debe ser exacta en el borde del teclado.
for style,depth,angle,horizon,width in [
    (0,38,20,10,116),(1,46,24,12,112),(1,100,65,50,40)
]:
    top,bottom=94.0,590.0
    H=bottom-top; center=640.0
    p=1.16 if style==1 else .86
    tilt=.82+.24*angle/65
    k=(.00016+.00108*depth/100)*p*tilt*100/width
    k=min(k,.42/H)
    horizonY=top+H*horizon/100
    C=(bottom-horizonY)*(1+k*H)/H
    def project(x,y):
        dt=bottom-y
        denom=1+k*dt
        return ((x+center*k*dt)/denom,
                (bottom+(bottom*k-C)*dt)/denom)
    for x in (16.,640.,1264.):
        px,py=project(x,bottom)
        assert abs(px-x)<.0001 and abs(py-bottom)<.0001
    for x in (16.,640.,1264.):
        px,py=project(x,top)
        assert abs(py-horizonY)<.0001
    L=project(16,top)[0];R=project(1264,top)[0]
    assert (R-L)/(1264-16)>=.70
try:
    from PIL import Image,ImageChops
except ImportError:
    Image=None
lab=Path('/tmp/linthesia-3d-geometry-qa')
modes=('clasico','moderno','extremo')
sizes=('1280','950','737')
if Image and all((lab/m/('paused-'+size+'.png')).is_file()
                 for m in modes for size in sizes):
    shots={(m,size):Image.open(lab/m/('paused-'+size+'.png')).convert('RGB')
           for m in modes for size in sizes}
    assert shots['clasico','1280'].size==(1280,720)
    assert shots['clasico','950'].size==(950,620)
    assert shots['clasico','737'].size==(737,540)
    for m in modes:
        im=shots[m,'1280']
        # Prohibir notas luminosas flotantes antes del borde visible.
        floating=sum(1 for y in range(108,137,2)
                       for x in range(18,1262,2)
                       if max(im.getpixel((x,y)))>150)
        assert floating==0,(m,floating)
        # El escenario llega hasta la unión con el teclado.
        assert im.getpixel((800,565))[0]>=25
    assert ImageChops.difference(
         shots['clasico','1280'].crop((20,142,1260,580)),
         shots['moderno','1280'].crop((20,142,1260,580))
    ).getbbox()
print('OK: 3D anclado, sin notas fuera del escenario; 3 perfiles, 3 tamaños')
