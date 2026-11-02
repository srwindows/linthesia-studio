#!/usr/bin/env bash
# Local quality gate for the future Linthesia fork; NEVER pushes to GitHub.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
printf '[1/5] Confirmando origen y licencia...\n'
git remote get-url origin | grep -Fx 'https://github.com/linthesia/linthesia.git'
test -s COPYING
grep -q 'GNU GENERAL PUBLIC LICENSE' COPYING
printf '[2/5] Excluyendo los volcados de ingeniería inversa y respaldos...\n'
git check-ignore -q analysis/seemusic_interop_20261021/SeeMusic_IL2CPP.rep/idata/00/~00000000.db/db.1.gbf
git check-ignore -q backups/absorcion_video_20261101/PlayingState.cpp
git check-ignore -q assets/scenarios/blender/Linthesia_Pack_7_Escenarios.blend1
printf '[3/5] Comprobando cambios de código y documentación...\n'
git diff --check
test -s README_ES.md && test -s RELEASE_CHECKLIST_ES.md
test -s .github/workflows/c-cpp.yml
printf '[4/5] Ejecutando todas las pruebas reproducibles...\n'
meson test -C build --print-errorlogs --num-processes 4
printf '[5/5] Comprobando staging limpio de distribución...\n'
binary=dist/staging/usr/bin/linthesia
scenes=dist/staging/usr/share/linthesia/graphics/scenarios
test -x "$binary"
test -s "$scenes/mesh_pianista_00.l3d"
test "$(find "$scenes" -maxdepth 1 -type f -name '*.l3d' | wc -l)" -ge 60
if ldd "$binary" | grep -q 'not found'; then
  echo 'ERROR: bibliotecas compartidas ausentes' >&2; exit 1
fi
printf 'VALIDADO LOCALMENTE. No equivale a prueba artística, hardware ni publicación.\n'
