"""Los dos controles silenciosos no se muestran tachados, pero siguen activos."""
from pathlib import Path
import sys
root=Path(sys.argv[1]);cpp=(root/'src/TrackSelectionState.cpp').read_text()
h=(root/'src/TrackSelectionState.h').read_text()
draw=cpp.split('void TrackSelectionState::Draw(Renderer &renderer) const {',1)[1]
logic=cpp.split('void TrackSelectionState::Update() {',1)[1].split('void TrackSelectionState::PlayTrackPreview',1)[0]
assert 'drawSilent(m_apply_all_you_play_silent,GetTexture(ButtonYouPlay)' in draw
assert 'drawSilent(m_apply_all_learning_silent,GetTexture(ButtonLearning)' in draw
assert 'GetTexture(ButtonYouPlaySilent)' not in draw
assert 'GetTexture(ButtonLearningSilent)' not in draw
assert 'const bool pressed=button.depressed || settling;' in draw
assert 'now-lastClick<180' in draw
assert 'renderer.DrawTga(symbol,button.x+dy/2,button.y+dy);' in draw
assert 'const int labelX=button.x+93;' in draw
assert 'm_you_play_silent_pulse=SDL_GetTicks();' in logic
assert 'm_learning_silent_pulse=SDL_GetTicks();' in logic
assert 'm_track_tiles[i].SetMode(Track::ModeYouPlaySilently);' in logic
assert 'm_track_tiles[i].SetMode(Track::ModeLearningSilently);' in logic
assert 'm_you_play_silent_pulse=0' in h
assert 'm_learning_silent_pulse=0' in h
assert 'Copyright (c) 2007 Nicholas Piegdon' in cpp
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
print('OK: iconos no tachados, funciones silenciosas y efecto visual al pulsar')
