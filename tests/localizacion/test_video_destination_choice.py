"""Destination is user-selected and remembered; no hardcoded save directory."""
from pathlib import Path
import sys
root=Path(sys.argv[1])
source=(root/'src/PlayingVideoExport.cpp').read_text()
header=(root/'src/PlayingState.h').read_text()
for needed in [
    'void PlayingState::ChooseVideoFolder()',
    'void PlayingState::ChooseVideoFile()',
    '"--getexistingdirectory"',
    '"--getsavefilename"',
    '"Elegir carpeta"',
    '"Guardar como..."',
    '"CARPETA DONDE SE GUARDARÁ"',
    '"NOMBRE DEL ARCHIVO"',
    '"Destino completo: "',
    'g_file_test(dir.c_str(),G_FILE_TEST_IS_DIR)',
    'RememberExportFolder(m_video_path);',
    'const std::string last=LastExportFolder();',
    'opts.path=WithFormat(m_video_path,m_video_format);',
    'access(Dirname(opts.path).c_str(),W_OK)!=0',
    'RememberExportFolder(opts.path);',
    'm_video_path=selected;',
    'm_video_path=dir+"/"+Basename(m_video_path);',
]:
    assert needed in source, needed
assert 'void ChooseVideoFolder();' in header
assert 'ChooseVideoFolder();' in source
assert 'ChooseVideoFile();' in source
assert source.index('  label(24,286,"CARPETA DONDE SE GUARDARÁ");') < source.index('  button(248,430,178,38,"Generar vídeo",true);')
assert 'g_get_user_config_dir()' in source
assert 'video-export.ini' in source
print('OK: independent native folder and Save As dialogs, directory persistence, writable path validation')
