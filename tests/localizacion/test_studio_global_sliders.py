"""Global home-screen settings and player use the same 3D particle values."""
from pathlib import Path
import sys
root=Path(sys.argv[1]);s=(root/'src/SettingsPanel.cpp').read_text();p=(root/'src/PlayingState.cpp').read_text()
fields=['effectWind','effectLife','effectTurbulence','effectInitialVelocity','effectBloom',
'effectOpacity','effectBlend','effectRays','effectSmoke','effectTrails','effectExplosion','effectFlameHeight']
for i,name in enumerate(fields):
 assert f'case {180+i}:value=a.{name};' in s
 assert f'case {180+i}:a.{name}=std::max(' in s
 assert f'add({180+i},' in s
 assert f'a.{name}=def.{name};' in s
 assert f'visual.{name}=p.{name};' in p
assert 'StudioEffectValue(m_draft_settings,row.id,current,minimum,maximum)' in s
assert 'SetStudioEffectValue(m_draft_settings,row.id,v)' in s
assert 'm_settings_status="Efecto 3D actualizado; pulsa Aplicar o Guardar.";' in s
assert 'continue; // don\'t draw a huge menu over a compact slider' in s
assert 'effectValue-effectMinimum' in s
print('OK: 12 real compact sliders in global settings, adjustable with Apply/Save/Restore and same values in player')
