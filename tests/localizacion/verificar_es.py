"""Prueba de localización integral de Linthesia 0.8.0 para Debian 13."""
from pathlib import Path
import io
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
 pristine=subprocess.run(['git','-C',str(root),'show',
                          f'origin/main:graphics/{name}.tga'],
                          capture_output=True,check=True).stdout
 baseline=Image.open(io.BytesIO(pristine))
 assert image.size==baseline.size,(name,'dimension mismatch')
 assert path.read_bytes()!=pristine,(name,'graphic untranslated')
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
 'src/TitleState.cpp':['Tamaño del teclado:', 'Salir de Linthesia.',
                       'Tamaño del teclado:', 'Salir de Linthesia.'],
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

with tempfile.TemporaryDirectory() as tmp:
 mo=Path(tmp)/'es.mo'
 result=subprocess.run(['msgfmt','--check','--statistics',
                        '-o',str(mo),str(root/'po/es.po')],
                       text=True,capture_output=True)
 assert result.returncode==0,(result.stdout,result.stderr)
 assert '39 mensajes traducidos' in result.stderr,result.stderr
 print('ES_GETTEXT_OK',result.stderr.strip())

# smoke-test the actual compiled executable, not just its translation catalog.
prefix=Path.home()/'.local/opt/linthesia'
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
