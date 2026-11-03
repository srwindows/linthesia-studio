"""Guard against effect speed being ignored and effects recoloring falling notes."""
from pathlib import Path
import re,sys
root=Path(sys.argv[1]);k=(root/'src/KeyboardDisplay.cpp').read_text();r=(root/'src/Renderer.cpp').read_text();p=(root/'src/PlayingState.cpp').read_text();s=(root/'src/SettingsPanel.cpp').read_text()
impact=r.split('void Renderer::DrawPianoImpactParticles(',1)[1].split('void Renderer::',1)[0]
notes=k.split('void KeyboardDisplay::DrawNotePass(',1)[1]
white=k.split('void KeyboardDisplay::DrawWhiteKeys(',1)[1].split('void KeyboardDisplay::DrawBlackKey(',1)[0]
black=k.split('void KeyboardDisplay::DrawBlackKeys(',1)[1].split('void KeyboardDisplay::DrawNotePass(',1)[0]
# Track color is still the source for 3D glass notes, white keys and black keys.
assert 'const SDL_Color base=Track::ColorNoteWhite[brush_id];' in notes
assert 'Effect3DPalette::Color(' not in notes.split('renderer.DrawNeonCapsule(',1)[0]
assert 'Effect3DPalette::Color(c,m_visual.effectPalette,' not in white
assert 'const SDL_Color accent=active||cue?\n               Track::ColorNoteWhite[color]' in black
# Only the independent colored impact structures receive preset swatches.
assert 'Effect3DPalette::Color(Track::ColorNoteWhite[find_result->second],' in white
assert 'Effect3DPalette::Color(Track::ColorNoteWhite[color],' in black
assert 'renderer.DrawPianoImpactParticles(impact.x,impact.y,impact.width,' in k
assert 'Color SOLO de efectos' in p and 'Color de efectos (no notas)' in s
# Every visual subsystem uses accelerated age, never unscaled age.
assert 'const float effectAge=elapsed*(m_effect_speed/100.f);' in impact
assert 'effectAge>=maxLife || releaseFade<=0.0f)return;' in impact
assert 'true,false,effectAge/lifeScale,' in impact
assert 'const float visualAge=effectAge/lifeScale;' in impact
assert 'if(m_effect_explosion && effectAge<ImpactFlameEmitter::Duration*lifeScale)' in impact
assert 'ImpactFlameEmitter::Sample(effectAge/lifeScale' in impact
assert 'const float age=effectAge/lifeScale;' in impact
assert 'const float time=effectAge-wait;' in impact
assert 'const float life=(effectAge-delay)/lifetime;' in impact
assert 'const float phase=(animationTime>=0.0f?animationTime:elapsed)*' in impact
assert 'flightAge*(m_visual.effectSpeed/100.f)>=1.10f*m_visual.effectLife/100.f' in k
# Keep animation timing independent from MIDI transport.
assert 'const float effectAge=elapsed*' not in notes
assert 'm_effect_speed=std::max(25,std::min(speed,200));' in (root/'src/Renderer.h').read_text()
assert 'visual.effectSpeed=p.effectSpeed;' in p
assert 'SetPlayerSetting(m_midi_preferences,e.id,value);' in p  # preview on every mouse movement
assert 'renderer.SetEffect3D(m_visual.effectSpeed,m_visual.effectCount,' in k
assert 'if(m_settings_open)DrawPlayerSettings(renderer);' in p
print('OK: 25-200% effect speed drives EVERY plume layer and expiry, separate palette for impact only; MIDI note/keyboard colors unchanged')
