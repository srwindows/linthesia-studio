"""Real SoundFont browsing, internal-only timbre overrides and safe persistence."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
src=(root/'src')
fluid=(src/'FluidEngine.cpp').read_text()
select=(src/'TrackSelectionState.cpp').read_text()
player=(src/'PlayingState.cpp').read_text()
persist=(src/'TrackSelectionPersistence.cpp').read_text()
tile=(src/'TrackTile.cpp').read_text()
header=(src/'TrackProperties.h').read_text()
for token in ('fluid_synth_get_sfont(synth,font)', 'fluid_sfont_iteration_next(sf)',
    'fluid_preset_get_banknum(preset)','fluid_preset_get_num(preset)',
    'fluid_preset_get_name(preset)','fluid_synth_program_select(synth,channel,',
    'renderMutex'):
 assert token in fluid,token
for token in ('OpenInstrumentPicker(int tile)','m_instrument_tile=tile',
    'm_state.midi_out->IsInternalFluid()', 'm_soundfont_presets=FluidEngine::Presets()',
    'm_track_tiles[m_instrument_tile].SetInstrument(item.bank,item.program,item.name)',
    'm_soundfont_presets[idx]', 'PersistCurrent()',
    'tile.SetInstrument(prop.instrument_bank,prop.instrument_program,',
    'm_state.track_properties[j].instrument_program=saved[j].program;'):
 assert token in select,token
assert 'm_instrument_hit=translated_mouse.released.left' in tile
assert 'InstrumentBank()' in select and 'InstrumentProgram()' in select
for token in ('instrument_program=-1','instrument_bank=0'):
 assert token in header
for token in ('"bank"','"program"','loaded.push_back({mode,color,retry!=0,program,bank})'):
 assert token in persist,token
for token in ('void PlayingState::ApplyTrackInstrument(size_t track,int channel)',
    'm_state.midi_out->IsInternalFluid()',
    'FluidEngine::SelectPreset(channel,bank,prog)',
    'if(overrideSF2 && ev.Type()==MidiEventType_ProgramChange)',
    'ApplyTrackInstrument(track_id,ev.Channel());'):
 assert token in player,token
print('OK: real sf2 bank/program names, per-track storage, internal FluidSynth only')
