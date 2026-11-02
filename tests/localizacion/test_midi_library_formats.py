"""MIDI library must enumerate formats supported by the parser."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
for name in ("SongLibState.cpp","MidiSearchIndex.cpp"):
 s=(root/"src"/name).read_text()
 for ext in (".mid",".midi",".rmi",".kar"):
  assert f'ext.c_str(),"{ext}"' in s,(name,ext)
reader=(root/"src/libmidi/Midi.cpp").read_text()
assert 'std::memcmp(form,"RMID",4)' in reader
assert 'const uint32_t chunkSize=length32(byteCount)' in reader
assert 'sizeof(unsigned long)' in reader  # regression note, no 64-bit stream.read
print("OK: library and global search find SMF, RMID, KAR")
