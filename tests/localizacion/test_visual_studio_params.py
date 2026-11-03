"""12 SeeMusic-reference inspired independent controls, with correct persistent linkage."""
from pathlib import Path
import sys
root=Path(sys.argv[1]);src=root/'src'
checks={
'advanced':(src/'AdvancedSettings.h').read_text(),
'config':(src/'AdvancedSettings.cpp').read_text(),
'visual':(src/'KeyboardDisplay.h').read_text(),
'playing':(src/'PlayingState.cpp').read_text(),
'keyboard':(src/'KeyboardDisplay.cpp').read_text(),
'renderer_h':(src/'Renderer.h').read_text(),
'renderer':(src/'Renderer.cpp').read_text()}
fields={'effectWind':'viento','effectLife':'vida','effectTurbulence':'turbulencia',
'effectInitialVelocity':'velocidad-inicial','effectBloom':'bloom',
'effectOpacity':'opacidad','effectBlend':'mezcla','effectRays':'rayos',
'effectSmoke':'humo','effectTrails':'estelas','effectExplosion':'explosion',
'effectFlameHeight':'altura-llama'}
for index,(key,setting) in enumerate(fields.items()):
 assert f' int {key}=' in checks['advanced']
 assert f' int {key}=' in checks['visual']
 assert f'"Efectos3D","{setting}"' in checks['config']
 assert f'I("Efectos3D","{setting}",s.{key});' in checks['config']
 assert f'visual.{key}=p.{key};' in checks['playing']
 assert f'case {706+index}: set(p.{key}' in checks['playing']
 assert f'saved.{key}!=m_midi_preferences.{key}' in checks['playing']
 assert f'm_visual.{key}' in checks['keyboard']
assert 'renderer.SetParticleStudio(' in checks['keyboard']
assert 'void SetParticleStudio(' in checks['renderer_h']
for effect in ('m_effect_wind','m_effect_turbulence','m_effect_life','m_effect_initial_velocity',
               'm_effect_bloom','m_effect_opacity','m_effect_blend','m_effect_rays',
               'm_effect_smoke','m_effect_trails','m_effect_explosion','m_effect_flame_height'):
 assert effect in checks['renderer']
assert 'if(m_effect_smoke)for(' in checks['renderer']
assert 'const SDL_Color base=Track::ColorNoteWhite[brush_id];' in checks['keyboard']
assert 'Effect3DPalette::Color(Track::ColorNoteWhite[find_result->second],' in checks['keyboard']
assert 'vec3 middle=mix(uTint,vec3(1.0,.93,.85),.24);' in checks['renderer']
assert 'if(m_effect_rays){' in checks['renderer']
assert 'if(m_effect_trails &&' in checks['renderer']
assert 'glBlendFunc(GL_SRC_ALPHA,m_effect_blend?GL_ONE:GL_ONE_MINUS_SRC_ALPHA);' in checks['renderer']
print('OK: 12 adjustable parameters persist and drive independent particle layers')
