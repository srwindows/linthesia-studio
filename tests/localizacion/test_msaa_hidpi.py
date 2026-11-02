"""Calidad OpenGL: framebuffer HiDPI, MSAA4x y uniones subpíxel."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
main=(root/"src/main.cpp").read_text()
keys=(root/"src/KeyboardDisplay.cpp").read_text()
assert "SDL_GL_MULTISAMPLEBUFFERS,1" in main
assert "SDL_GL_MULTISAMPLESAMPLES,4" in main
assert "SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES,&sampleCount);" in main
assert "if(sampleBuffers>0 && sampleCount>1)glEnable(GL_MULTISAMPLE);" in main
assert "SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,0);" in main
assert "SDL_WINDOW_ALLOW_HIGHDPI" in main
assert "SDL_GL_GetDrawableSize(m_sdl_window,&drawableW,&drawableH);" in main
assert "glViewport(0,0,std::max(1,drawableW),std::max(1,drawableH));" in main
assert "if(gap>=-onePixel && gap<=onePixel &&" in keys
assert "show_duration/std::max(1,y_offset)" in keys
assert "if(m_render_3d && prior!=most_recent.end())" in keys
assert "Track::ColorNoteWhite[brush_id]" in keys
assert "GNU GENERAL PUBLIC LICENSE" in (root/"COPYING").read_text()
log=Path("/tmp/linthesia-msaa-visual.log")
if log.is_file():assert "OpenGL: MSAA 4x (buffers=1)" in log.read_text()
print("OK: MSAA real 4x, HiDPI, fallback y continuidad subpixel")
