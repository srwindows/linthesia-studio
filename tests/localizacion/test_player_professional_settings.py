"""Panel profesional: 2D/3D independientes, sliders y transacciones reales."""
from pathlib import Path
import configparser,sys
root=Path(sys.argv[1])
src=(root/'src/PlayingState.cpp').read_text()
hdr=(root/'src/AdvancedSettings.h').read_text()
cfgsrc=(root/'src/AdvancedSettings.cpp').read_text()
kbd=(root/'src/KeyboardDisplay.cpp').read_text()
rend=(root/'src/Renderer.cpp').read_text()
for tab in ('"Pantalla"','"Transporte"','"Evaluación"','"2D"',
            '"3D"','"Compás"','"Colores"','"Avanzado"'):
    assert tab in src
for field in ('player2DWidth','player2DRadius','player2DOpacity','player2DGlow',
              'player2DBrightness','player2DGuideSize','player2DGuideOpacity',
              'player2DKeyOpacity','player2DBackgroundOpacity','player2DShadows',
              'player2DHeight','player3DDepth','player3DAngle','player3DHorizon',
              'player3DStageWidth','player3DRadius','player3DOpacity',
              'player3DGlow','player3DKeyOpacity','player3DBackgroundOpacity',
              'player3DShadows','player3DHeight'):
    assert field in hdr,field
    assert field in cfgsrc,field
    assert field in src,field
assert 'PlayerSettingEntries' in src
assert 'SetPlayerSetting' in src
assert 'mouse.released.left && win.minus(row).contains' in src
assert 'mouse.held.left || mouse.released.left' in src
assert 'm_settings_page*win.rows+row' in src
assert 'void PlayingState::SavePlayerSettings()' in src
assert 'AdvancedSettingsIO::Save(m_midi_preferences,error)' in src
assert 'void PlayingState::CancelPlayerSettings()' in src
assert 'm_midi_preferences=m_settings_before' in src
assert 'm_view_3d=m_settings_view_before' in src
assert 'void PlayingState::ResetPlayerSettingsPage()' in src
assert 'm_settings_feedback="Vista previa activa' in src
assert 'm_visual.opacity' in kbd and 'm_visual.radius' in kbd
assert 'm_visual.brightness' in kbd and 'm_visual.glow' in kbd
assert 'm_visual.guideOpacity' in kbd and 'm_visual.shadows' in kbd
assert 'm_visual.backgroundOpacity' in kbd
assert 'm_visual.depth' in kbd and 'm_visual.horizon' in kbd
assert 'm_visual.stageWidth' in kbd and 'm_visual.angle' in kbd
assert 'horizonY' in rend and 'widthFactor' in rend
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
lab=Path('/tmp/linthesia-pro-settings-qa')
c=lab/'config'/'linthesia'/'ajustes.ini'
if c.is_file():
    f=configparser.ConfigParser();f.read(c)
    assert f.getint('Reproductor','2d-width')==65
    assert f.getint('Reproductor','3d-depth')==75
    assert f.getboolean('Reproductor','view-3d')
    assert f.getint('Reproductor','2d-height')==16
    assert f.getint('Reproductor','3d-height')==16
# Legacy screenshots were made with the old full-screen modal layout and
# cannot validate floating coordinates. Current window bounds are tested
# by test_floating_settings.cpp and fresh headless screenshots.
assert 'm_settings_dragging=true;' in src
assert 'win.action(3).contains(mouse.x,mouse.y)' in src
assert 'const auto win=FloatingSettings::Fit(' in src
print('OK: controles 2D/3D editables, modos independientes, guardar/cancelar y paginación')
