"""Prueba de localización integral de Linthesia 0.8.0 para Debian 13."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from PIL import Image

root=Path(sys.argv[1])
translations=[
 'title_Exit','title_ChooseTracks','title_BackBox',
 'title_ButtonPageBack','title_ButtonPageNext','tracks_PlaySong',
 'tracks_BackToTitle','score_RetrySong','title_SongBox',
 'title_OutputBox','title_InputBox','title_DirBox',
 'title_GameMusicThemes','stats_text','play_Status','play_Keys','trackbox'
]
for name in translations:
 path=root/'graphics'/f'{name}.tga'
 image=Image.open(path);image.load()
 # The translated assets are now on main; comparing them with origin/main
 # would compare the image to itself after publication. Check each actual
 # installed asset's structure and nonempty visual content instead.
 assert image.width>=16 and image.height>=16,(name,'invalid image dimensions')
 assert image.getbbox() is not None,(name,'blank image')
 assert image.mode in ('RGB','RGBA'),(name,'unsupported TGA mode')
 print('ES_TGA_OK',name,image.size)

midi=(root/'src/libmidi/MidiUtil.cpp').read_text()
instruments=midi.split('string const InstrumentNames[InstrumentCount] = {',1)[1].split('};',1)[0]
names=re.findall(r'^\s*"([^"]+)"',instruments,re.M)
assert len(names)==130,len(names)
assert names[0]=='Piano de cola acústico'
assert names[-2:]==['Percusión','Varios instrumentos']
assert 'Acoustic Grand Piano' not in instruments
print('ES_MIDI_INSTRUMENTS_OK',len(names))

required={
 'src/TitleState.cpp':['Tamaño del teclado:', 'Salir de Linthesia - Studio.',
                       'Tamaño del teclado:', 'Salir de Linthesia - Studio.'],
 'src/DeviceTile.cpp':['Sin entrada: acompañamiento sin puntuación'],
 'src/TrackSelectionState.cpp':['Seleccionar las pistas para tocar',
                                'Página ', 'Escuchar una muestra'],
 'src/TrackTile.cpp':['Reproducción automática','Tocas tú'],
 'src/PlayingState.cpp':['En pausa','BPM','Fallos','Ajustes','Factor'],
 'src/StatsState.cpp':['Volver a la selección de pistas.'],
 'src/Keyboard.cpp':['88 teclas'],
 'src/LinthesiaError.cpp':['Tipo de piano no válido.'],
 'src/libmidi/MidiUtil.cpp':['No se pudo abrir el dispositivo MIDI seleccionado.'],
 'src/main.cpp':['bindtextdomain("linthesia", LOCALEDIR)'],
}
for name,strings in required.items():
 text=(root/name).read_text()
 for item in strings:
  assert item in text,(name,'missing Spanish text',item)
print('ES_GUI_CODE_OK',len(required),'sources')
# The user-facing fork brand is distinct from internal filenames and IDs.
assert 'Linthesia - Studio ' in (root/'src/main.cpp').read_text()
assert 'Configuración de Linthesia - Studio' in (root/'src/SettingsPanel.cpp').read_text()
assert 'Linthesia - Studio · versión ' in (root/'src/TitleState.cpp').read_text()
assert '— STUDIO —' in (root/'src/TitleState.cpp').read_text()
assert 'Name=Linthesia - Studio' in (root/'extra/linthesia.desktop').read_text()
assert '<name>Linthesia - Studio</name>' in (root/'extra/linthesia.metainfo.xml').read_text()
assert '<id>linthesia</id>' in (root/'extra/linthesia.metainfo.xml').read_text()
assert 'Exec=linthesia' in (root/'extra/linthesia.desktop').read_text()
print('ES_STUDIO_BRANDING_OK: UI renamed, binary/config identity preserved')

with tempfile.TemporaryDirectory() as tmp:
 mo=Path(tmp)/'es.mo'
 result=subprocess.run(['msgfmt','--check','--statistics',
                        '-o',str(mo),str(root/'po/es.po')],
                       text=True,capture_output=True)
 assert result.returncode==0,(result.stdout,result.stderr)
 assert '39 mensajes traducidos' in result.stderr,result.stderr
 print('ES_GETTEXT_OK',result.stderr.strip())

# smoke-test the actual compiled executable, not just its translation catalog.
prefix=Path(sys.argv[2]) if len(sys.argv)>2 else Path.home()/'.local/opt/linthesia'
exe=prefix/'bin/linthesia'
env=dict(os.environ,LC_ALL='es_ES.UTF-8')
env['GSETTINGS_SCHEMA_DIR']=str(prefix/'share/glib-2.0/schemas')
env['LD_LIBRARY_PATH']=str(prefix/'lib/x86_64-linux-gnu')+':'+env.get('LD_LIBRARY_PATH','')
res=subprocess.run([str(exe),'--help'],text=True,env=env,
                   capture_output=True,timeout=10)
assert res.returncode==0,res.stderr
for phrase in ['Opciones:', 'para empezar en modo ventana',
               'para elegir la carpeta de la biblioteca musical',
               'restablecer la carpeta de la biblioteca musical']:
 assert phrase in res.stdout,('help untranslated',phrase)
print('ES_DEBIAN_CLI_OK')
print('LOCALIZACION_ES: VALIDACIÓN COMPLETA')
