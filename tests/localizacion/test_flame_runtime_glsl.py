"""Compile the REAL cinematic GLSL shader in Mesa, not just the C++ wrapper.

This would have caught the invalid float/vec3 redeclaration previously shipped.
"""
import configparser
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

root = Path(sys.argv[1])
binary = Path(sys.argv[2])
prefix = Path(sys.argv[3]) if len(sys.argv)>3 else Path('/usr')
if not binary.is_file():
    raise RuntimeError(f'Executable missing: {binary}')
for name in ('Xvfb', 'xdotool'):
    if not shutil.which(name):
        raise RuntimeError(f'Real GLSL check requires {name}')
fixture = root / 'music' / 'Hannon' / 'Lesson_1.mid'
if not fixture.is_file():
    raise RuntimeError(f'MIDI fixture missing: {fixture}')

display = next((f':{n}' for n in range(85, 94)
                if not Path(f'/tmp/.X11-unix/X{n}').exists()), None)
if not display:
    raise RuntimeError('No free Xvfb display')
with tempfile.TemporaryDirectory(prefix='linthesia-glsl-') as tmp:
    temp = Path(tmp)
    confdir = temp / 'config' / 'linthesia'
    confdir.mkdir(parents=True)
    config = configparser.ConfigParser(interpolation=None)
    # Standalone config avoids physical MIDI / FANTOM devices.
    config['MIDI'] = {'input': 'Desactivado', 'output': 'FluidSynth interno',
                      'key-lights': 'false'}
    config['Reproductor'] = {'view-3d': 'true', 'style-3d': '2',
                             'notation-visible': 'false'}
    config['Sonido'] = {'device': 'default'}
    config['FluidSynth'] = {'extra-soundfonts': ''}
    with (confdir / 'ajustes.ini').open('w') as f:
        config.write(f)
    env = os.environ.copy()
    env.update({'DISPLAY': display,
                'SDL_VIDEODRIVER': 'x11',
                'LIBGL_ALWAYS_SOFTWARE': '1',
                'XDG_CONFIG_HOME': str(temp / 'config'),
                'XDG_DATA_HOME': str(temp / 'data')})
    # Do not rely on a developer's home directory. Meson passes the
    # install prefix explicitly, so CI can test a clean staged install.
    libdir = prefix / 'lib' / 'x86_64-linux-gnu'
    env['LD_LIBRARY_PATH'] = f"{libdir}:{env.get('LD_LIBRARY_PATH', '')}"
    schema_dir=prefix/'share/glib-2.0/schemas'
    if not (schema_dir/'gschemas.compiled').is_file():
        # Running directly from a development tree also works: compile
        # only the source schema inside this disposable test directory.
        import shutil
        schema_dir=temp/'schemas'
        schema_dir.mkdir()
        src_schema=root/'extra/com.github.linthesia.linthesia.gschema.xml'
        shutil.copyfile(src_schema,schema_dir/src_schema.name)
        subprocess.run(['glib-compile-schemas',str(schema_dir)],check=True)
    env['GSETTINGS_SCHEMA_DIR'] = str(schema_dir)
    with (temp/'xvfb.log').open('wb') as xlog, (temp/'app.log').open('wb') as alog:
        xvfb = subprocess.Popen(['Xvfb', display, '-screen', '0',
                         '1280x720x24', '+extension', 'GLX', '-nolisten', 'tcp'],
                         stdout=xlog, stderr=subprocess.STDOUT)
        app = None
        try:
            time.sleep(.45)
            app = subprocess.Popen([str(binary), '-w', '-f', str(fixture)],
                                  env=env, stdout=alog, stderr=subprocess.STDOUT)
            time.sleep(1.15)
            for _ in range(2):
                subprocess.run(['xdotool', 'mousemove', '1160', '676', 'click', '1'],
                               env=env, check=True, capture_output=True)
                time.sleep(.4)
            for _ in range(35):
                time.sleep(.15)
                text = (temp/'app.log').read_text(errors='replace')
                if 'Linthesia GLSL flame: shader activo' in text:
                    break
                if 'GLSL flame compile error' in text or 'GLSL flame link error' in text:
                    raise AssertionError(f'GLSL compile/link failed:\n{text[-2500:]}')
            else:
                raise AssertionError('GLSL flame did not initialize in a real GL context: '
                                     + text[-1100:])
            assert 'GLSL flame compile error' not in text, text[-2500:]
            assert 'GLSL flame link error' not in text, text[-2500:]
            print('OK: cinematic flame GLSL compiled and linked in real Mesa OpenGL')
        finally:
            if app:
                app.terminate()
                try:
                    app.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    app.kill()
                    app.wait()
            xvfb.terminate()
            try:
                xvfb.wait(timeout=2)
            except subprocess.TimeoutExpired:
                xvfb.kill()
                xvfb.wait()
