#!/bin/sh
# Detects the stale-MIDI-library regression that exited Linthesia with code 127.
set -eu
if [ "$#" -gt 0 ]; then prefix="$1"; else prefix="$HOME/.local/opt/linthesia"; fi
exe="$prefix/bin/linthesia"
lib="$prefix/lib/x86_64-linux-gnu/liblibmidi.so"
launcher="$HOME/.local/bin/linthesia"
test -x "$exe"
test -f "$lib"
test -x "$launcher"
if grep -E '^(export LD_LIBRARY_PATH=.*(linthesia-3d-20261021|linthesia-professional-20261018))' "$launcher";then
 echo 'ERROR: launcher still prioritizes obsolete libmidi' >&2
 exit 1
fi
if ! nm -D -C "$lib" |grep -Fq 'Midi::GetCurrentKeyAccidentals() const';then
 echo 'ERROR: installed library lacks Midi::GetCurrentKeyAccidentals()' >&2
 exit 1
fi
if ! readelf -d "$exe" |grep -Fq '$ORIGIN/../lib/x86_64-linux-gnu';then
 echo 'ERROR: executable lacks portable bundled runtime path' >&2
 exit 1
fi
if env -u LD_LIBRARY_PATH ldd -r "$exe" 2>&1 |grep -E 'undefined symbol|not found';then
 echo 'ERROR: executable has unresolved shared-library dependencies' >&2
 exit 1
fi
echo 'OK: launcher and executable load the matching liblibmidi.so; no unresolved symbols'
