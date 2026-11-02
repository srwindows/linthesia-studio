"""Off-line loudness calibration. No ALSA/PipeWire/hardware device opened."""
import ctypes as C
import ctypes.util
from array import array
from math import log10,sqrt
from pathlib import Path
import sys
lib_name=ctypes_lib=ctypes.util.find_library('fluidsynth')
bank=Path('/usr/share/sounds/sf2/FluidR3_GM.sf2')
limiter=Path('/usr/lib/ladspa/fast_lookahead_limiter_1913.so')
if not lib_name or not bank.is_file() or not limiter.is_file():
    print('SKIP: Linux-specific FluidSynth, SF2 or LADSPA limiter missing')
    sys.exit(77)
f=C.CDLL(ctypes_lib)
P=C.c_void_p
def fn(name,ret,args):
    call=getattr(f,name);call.restype=ret;call.argtypes=args;return call
settings=fn('new_fluid_settings',P,[])
set_num=fn('fluid_settings_setnum',C.c_int,[P,C.c_char_p,C.c_double])
set_int=fn('fluid_settings_setint',C.c_int,[P,C.c_char_p,C.c_int])
synth=fn('new_fluid_synth',P,[P])
fx_get=fn('fluid_synth_get_ladspa_fx',P,[P])
add=fn('fluid_ladspa_add_effect',C.c_int,[P,C.c_char_p,C.c_char_p,C.c_char_p])
link=fn('fluid_ladspa_effect_link',C.c_int,[P,C.c_char_p,C.c_char_p,C.c_char_p])
ctl=fn('fluid_ladspa_effect_set_control',C.c_int,[P,C.c_char_p,C.c_char_p,C.c_float])
activate=fn('fluid_ladspa_activate',C.c_int,[P])
sf=fn('fluid_synth_sfload',C.c_int,[P,C.c_char_p,C.c_int])
cc=fn('fluid_synth_cc',C.c_int,[P,C.c_int,C.c_int,C.c_int])
on=fn('fluid_synth_noteon',C.c_int,[P,C.c_int,C.c_int,C.c_int])
write=fn('fluid_synth_write_float',C.c_int,[P,C.c_int,P,C.c_int,C.c_int,P,C.c_int,C.c_int])
del_s=fn('delete_fluid_synth',None,[P])
del_st=fn('delete_fluid_settings',None,[P])
def measure(boost):
    st=settings()
    assert st
    assert set_num(st,b'synth.sample-rate',48000.0)==0
    assert set_num(st,b'synth.gain',0.5)==0
    assert set_int(st,b'synth.ladspa.active',1)==0
    sy=synth(st)
    assert sy
    fx=fx_get(sy)
    assert fx
    assert add(fx,b'limiter',str(limiter).encode(),b'fastLookaheadLimiter')==0
    for a,b in [(b'Input 1',b'Main:L'),(b'Input 2',b'Main:R'),(b'Output 1',b'Main:L'),(b'Output 2',b'Main:R')]:
        assert link(fx,b'limiter',a,b)==0
    for param,value in [(b'Input gain',boost),(b'Limit',-1.5),(b'Release',0.12)]:
        assert ctl(fx,b'limiter',param,value)==0
    assert activate(fx)==0
    assert sf(sy,str(bank).encode(),1)>=0
    assert cc(sy,0,7,110)==0
    assert on(sy,0,60,90)==0
    total,energy,peak=0,0.0,0.0
    for i in range(12):
        n=4096
        left=(C.c_float*n)()
        right=(C.c_float*n)()
        assert write(sy,n,C.cast(left,P),0,1,C.cast(right,P),0,1)==0
        for channel in (left,right):
            for sample in channel:
                total+=1
                energy+=sample*sample
                peak=max(peak,abs(sample))
    del_s(sy)
    del_st(st)
    return 20*log10(max(sqrt(energy/total),1e-12)),peak
results={x:measure(x) for x in (0,6,12,18,24)}
for gain,(rms,peak) in results.items():
    print(f'boost {gain:2} dB: RMS={rms:.2f} dBFS, peak={20*log10(peak):.2f} dBFS')
assert results[18][0]-results[6][0]>10
assert results[18][0]>-16.5
assert all(p<0.90 for _,p in results.values())
print('OK: +18 dB raises quiet FluidR3 piano by >10 dB and all peaks stay limited')
