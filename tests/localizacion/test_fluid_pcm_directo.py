"""Real pipewire/pulse loopback level regression, no physical audio devices."""
import math
import os
import shutil
import subprocess
import sys
import tempfile
import time
import wave
from array import array
from pathlib import Path

binary = sys.argv[1]
sf = Path('/usr/share/sounds/sf2/FluidR3_GM.sf2')
tools = ("pactl", "ffmpeg")
if not sf.exists() or not all(shutil.which(x) for x in tools):
    print("SKIP: SF2/ffmpeg/PulseAudio client not available")
    sys.exit(77)
if subprocess.run(["pactl", "info"], capture_output=True).returncode != 0:
    print("SKIP: PulseAudio server not available")
    sys.exit(77)
sink = "alsa_output.usb-Roland_FANTOM_NULL_TEST_" + str(os.getpid())
try:
    module = subprocess.check_output([
        "pactl", "load-module", "module-null-sink", "sink_name=" + sink,
        "rate=48000", "channels=2",
        "sink_properties=device.description=LINTHESIA_NULL_TEST_NO_HARDWARE",
    ], text=True, stderr=subprocess.STDOUT, timeout=8).strip()
except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as ex:
    print("SKIP: null sink not available:", ex)
    sys.exit(77)
try:
    with tempfile.TemporaryDirectory() as folder:
        audio = str(Path(folder) / "pcm.wav")
        recorder = subprocess.Popen(
            ["ffmpeg", "-hide_banner", "-loglevel", "error", "-nostdin",
             "-y", "-f", "pulse", "-i", sink + ".monitor",
             "-t", "3.5", "-ar", "48000", "-ac", "2",
             "-c:a", "pcm_s16le", audio],
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        time.sleep(0.2)
        engine = subprocess.run([binary, sink], text=True, capture_output=True,
                                timeout=15)
        stderr = recorder.communicate(timeout=12)[1].decode(errors="replace")
        assert engine.returncode == 0, engine.stdout + engine.stderr
        assert recorder.returncode == 0, stderr
        with wave.open(audio, "rb") as f:
            assert f.getframerate() == 48000 and f.getnchannels() == 2
            assert f.getsampwidth() == 2
            signal = array('h')
            signal.frombytes(f.readframes(f.getnframes()))
        # Compare against the direct render's -19 dBFS peak. The defective
        # libfluidsynth PulseAudio backend delivered -55.5 dBFS instead.
        peak = max(abs(x) for x in signal) / 32768.0
        dbfs = 20 * math.log10(max(peak, 1e-12))
        assert -25 < dbfs < -10, \
            f"Wrong real-time PCM level: {dbfs:.2f} dBFS (expected -19)"
        print(f"OK: PulseAudio real-time PCM peak {dbfs:.2f} dBFS, "
              "no hardware output, no booster, thread stopped")
finally:
    subprocess.run(["pactl", "unload-module", module],
                   capture_output=True, timeout=8)
