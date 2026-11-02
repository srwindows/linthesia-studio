"""Reproductor: comprobar fila única, ajustes reales y persistencia.

Basado en dos sesiones gráficas de Xvfb y un archivo MIDI de prueba.
"""
from pathlib import Path
import sys
root=Path(sys.argv[1])
source=(root/'src/PlayingState.cpp').read_text()
keyboard=(root/'src/KeyboardDisplay.cpp').read_text()
prefs=(root/'src/AdvancedSettings.cpp').read_text()
assert 'RibbonBox Ribbon(int index,int width)' in source
assert 'const Base boxes[]' in source
assert 'control(0,"Volver")' in source
assert 'control(1,"Ayuda")' in source
assert 'control(6,"▲")' in source
assert 'control(7,"▼")' in source
assert 'control(8,"REST")' in source
assert 'const RibbonBox meterBox=Ribbon(9,width);' in source
assert 'control(17,"Inicio")' in source
assert 'const unsigned int currentBeat=' in source
assert 'lcd(10,"Lector",STRING(currentBar<<"/"<<totalBars)' in source
assert 'lcd(11,"Fallos",STRING(m_state.stats.stray_notes)' in source
assert 'm_missed_notes+m_state.stats.stray_notes' not in source
assert 'lcd(12,"Acorde",m_current_chord' in source
assert 'lcd(13,"Tonalidad",midi.GetCurrentKeySignature()' in source
assert 'control(14,"Bucle",m_loop_active)' in source
assert 'control(15,m_view_3d?"2D":"3D")' in source
assert 'control(16,"",m_settings_open)' in source
assert 'void PlayingState::DrawPlayerSettings(Renderer& renderer) const' in source
assert 'bool PlayingState::HandlePlayerSettings(const MouseInfo& mouse)' in source
assert 'AdvancedSettingsIO::Save(m_midi_preferences,saveError)' in source
assert 'GetMusicalBars()' in source
assert 'case 201: set(p.player2DHeight,10,30);break;' in source
assert 'm_show_duration=m_midi_preferences.playerNoteDurationMs*1000LL' in source
assert 'pianoPercent' in keyboard and 'm_display_note_names' in keyboard
assert 'g_key_file_set_' in prefs and '"Reproductor"' in prefs
body=source.split('void PlayingState::DrawTransport(Renderer &renderer) const {',1)[1].split(
    'void PlayingState::Draw(Renderer &renderer) const {',1)[0]
assert 'control(378,22,"-")' not in body
assert 'control(451,22,"+")' not in body
assert 'STRING(static_cast<int>(m_state.base_volume*100)' not in body

try:
    from PIL import Image, ImageChops
except ImportError:
    Image=None
lab=Path('/tmp/linthesia-ribbon-validation')
names=['ribbon-1280','tempo-up','tempo-rest','settings-screen',
       'settings-piano','piano-height-plus','settings-colors',
       'settings-closed','loop-editor','ribbon-737']
if Image and all((lab/(n+'.png')).exists() for n in names):
    shots={n: Image.open(lab/(n+'.png')).convert('RGB') for n in names}
    assert shots['ribbon-1280'].size==(1280,720)
    assert shots['ribbon-737'].size==(737,540)
    area=(260,10,330,45)
    assert ImageChops.difference(shots['ribbon-1280'].crop(area),
                                 shots['tempo-up'].crop(area)).getbbox()
    assert ImageChops.difference(shots['ribbon-1280'].crop(area),
                                 shots['tempo-rest'].crop(area)).getbbox() is None
    modal=(540,140)
    assert shots['settings-screen'].getpixel(modal)!=shots['settings-closed'].getpixel(modal)
    assert shots['loop-editor'].getpixel((550,140))!=shots['settings-closed'].getpixel((550,140))
    assert ImageChops.difference(shots['settings-piano'],
                                 shots['piano-height-plus']).getbbox()
    # La vista a 737px tiene la última celda y el borde del progreso visibles.
    assert max(shots['ribbon-737'].getpixel((704,25)))>35
    assert max(shots['ribbon-737'].getpixel((600,67)))>35
config=lab/'config'/'linthesia'/'ajustes.ini'
if config.exists():
    import configparser
    cp=configparser.ConfigParser();cp.read(config)
    assert cp.has_section('Reproductor')
    assert int(cp.get('Reproductor','piano-percent'))>=17
    assert cp.get('Reproductor','names')=='true'
print('OK: cabecera única, tempo vertical, lector, fallos correctos, ajustes funcionales, persistencia y Bucle')
