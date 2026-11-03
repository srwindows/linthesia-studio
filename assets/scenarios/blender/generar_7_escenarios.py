from pathlib import Path
from os import environ as _linthesia_environ
_LINTHESIA_ROOT=Path(_linthesia_environ.get('LINTHESIA_ROOT', next((str(x) for x in Path(__file__).resolve().parents if (x/'meson.build').exists()),'.')))
# Linthesia — generator of updated seven Blender scenarios.
# Preserves existing editable scene artwork. Rebuilds the scaled grand piano.
# The previous primitive generator is retained as LEGACY_20261022.py.
from pathlib import Path
import runpy
root=Path(__file__).resolve().parent
runpy.run_path(str(root/'grand_3d_export.py'),run_name='__main__')
# Render/rebake previews separately using render_pack_grand.py.
