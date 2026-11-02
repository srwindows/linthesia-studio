"""Score pagination, keyboard-safe layout, and GLib initialization regressions."""
from pathlib import Path
import sys
r=Path(sys.argv[1])
p=(r/'src/PlayingState.cpp').read_text()
h=(r/'src/PlayingState.h').read_text()
u=(r/'src/UserSettings.cpp').read_text()
for token in (
    'g.systems=g.height>=430?2:1',
    'g.height=std::max(340,GetStateHeight()-250)',
    'g.systemDelta=g.systems==2?g.height/2:0',
    'g.bottom[0]=g.top+g.height*(g.systems==2?17:33)/100',
    'g.bottom[1]=g.top+g.height*(g.systems==2?37:67)/100',
    'view.SystemAt(visual,scoreStart)',
    'view.XAt(visual,scoreStart)',
    'view.Bottom(part,sys)',
    'm_staff_page_start=-1;',
    'm_staff_page_start=std::min<microseconds_t>(end,base+view.horizon)',
    'm_staff_page_start=-1; // follow the current playback position again',
    'view.SystemAt(a.start,scoreStart)!=view.SystemAt(b.start,scoreStart)',
    'view.Bottom(0,sys)-8*view.step-19:top+20'):
    assert token in p, token
for token in ('int systems=1,systemDelta=0','int SystemAt(', 'int XAt(', 'int Bottom('):
    assert token in h,token
assert 'Gio::init();' in u
assert u.index('Gio::init();') < u.index('settings = Gio::Settings::create(')
print("OK: dual-system score pages, follow, keyboard clearance, accurate editor alignment and Gio init")
