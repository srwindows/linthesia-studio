from pathlib import Path
import sys
p=Path(sys.argv[1]);r=(p/'src/Renderer.cpp').read_text();k=(p/'src/KeyboardDisplay.cpp').read_text()
s=(p/'src/SettingsPanel.cpp').read_text();pl=(p/'src/PlayingState.cpp').read_text()
a=(p/'src/AdvancedSettings.cpp').read_text()
assert 'const bool birthing=cinematic && m_effect_birth==1 && birthAge>=0.0f;' in r
assert 'TopBirthEmitter::Reveal(birthClock)' in r
assert 'if(birthing && reveal<=.001f)' in r
assert 'glScissor(left,low,std::max(0,right-left),std::max(0,high-low));' in r
assert 'if(!hadScissor)glDisable(GL_SCISSOR_TEST);' in r
assert 'birthClock<TopBirthEmitter::Duration' in r
assert 'DrawTopBirthBurst(x,birthTop,w,base,birthAge,birthSeed,glow);' in r
assert 'TopBirthEmitter::Sample(time,i,seed,float(w),' in r
assert 'float flow=(uBirth>.5?uv.y:(uv.y-1.0))' in r
assert 'uniform float uBirthAge;' in r
assert 'if(uBirth>.5 && uBirthAge>=0.0 && uBirthAge<1.2)' in r
assert 'm_effect_birth==1,' in r
assert 'birthAge>=0?birthAge*m_effect_speed/100.0f:-1.0f' in r
assert 'm_visible_births.emplace(identity,' in k
assert 'm_effect_time-result.first->second.first' in k
assert 'std::max(y,top)' in k
assert 'm_visual.effectSpread,m_visual.effectSize,m_visual.effectBirth' in k
assert '"Explosión superior → fuego descendente"' in s
assert 'case 705: set(p.effectBirth,0,1);' in pl
assert '"Efectos3D","nacimiento"' in a
assert 'DrawPianoImpactParticles(impact.x,impact.y,impact.width,' in k
print('OK: top emitter / downward GLSL / UI / MIDI and impact wiring')
