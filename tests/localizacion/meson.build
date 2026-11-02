localizacion_py = find_program('python3')
test('localizacion-es-ES', localizacion_py,
 args: [files('verificar_es.py'), meson.project_source_root()],
 timeout: 40)
test('botones-inicio-y-cursor', localizacion_py,
 args: [files('test_botones_inicio.py'), meson.project_source_root()],
 timeout: 30)
folder_regression = executable('test-personal-folders',
  'test_personal_folders.cpp', '../../src/PersonalFolders.cpp',
  include_directories: include_directories('../../src'),
  dependencies: dependency('glib-2.0'), install: false)
test('carpetas-personales-seguridad', folder_regression)
drop_choices_regression=executable('test-drop-choices',
  'test_drop_choices.cpp','../../src/PersonalFolders.cpp',
  include_directories:include_directories('../../src'),
  dependencies:dependency('glib-2.0'),install:false)
test('carpetas-soltar-aqui-o-dentro',drop_choices_regression)
virtual_folders_regression=executable('test-virtual-folder-placements',
 'test_folder_placements.cpp','../../src/PersonalFolders.cpp',
 '../../src/FolderPlacements.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('carpetas-originales-y-personales',virtual_folders_regression)
return_main_regression=executable('test-return-to-main',
 'test_return_to_main.cpp','../../src/PersonalFolders.cpp',
 '../../src/FolderPlacements.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('devolver-carpetas-a-principales',return_main_regression)
midi_list_layout_test=executable('test-midi-list-layout',
 'test_midi_list_layout.cpp',
 include_directories:include_directories('../../src'),install:false)
test('biblioteca-carpetas-y-lista-midi',midi_list_layout_test)
create_in_original_regression=executable('test-create-in-original',
 'test_create_in_original.cpp','../../src/PersonalFolders.cpp',
 '../../src/FolderPlacements.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('crear-subcarpetas-en-cualquier-carpeta',create_in_original_regression)

global_midi_search_test=executable('test-midi-global-search',
 'test_midi_search.cpp',
 '../../src/MidiSearchIndex.cpp',
 '../../src/PersonalFolders.cpp',
 '../../src/FolderPlacements.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('busqueda-midi-global-y-rutas',global_midi_search_test)

settings_persistence_test=executable('test-advanced-settings',
 'test_advanced_settings.cpp','../../src/AdvancedSettings.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('configuracion-persistencia-real',settings_persistence_test)

fluid_output_test=executable('test-fluid-audio',
 'test_fluid_audio.cpp', dependencies:dependency('fluidsynth'),
 install:false)
test('fluidsynth-genera-audio-real',fluid_output_test)

fluid_engine_driver_test=executable('test-fluid-engine-alsa-null',
 'test_fluid_engine_output.cpp','../../src/FluidEngine.cpp',
 include_directories:include_directories('../../src'),
 link_with:libmidi,
 dependencies:[dependency('fluidsynth'),dependency('libpulse-simple'),dependency('threads')],
 install:false)
test('fluidsynth-motor-alsa-sin-hardware',fluid_engine_driver_test)
ui_appearance_test=executable('test-ui-appearance','test_ui_appearance.cpp',
 '../../src/UiAppearance.cpp',
 include_directories:include_directories('../../src'),install:false)
test('escalado-y-temas-globales',ui_appearance_test)

midi_activity_test=executable('test-midi-activity','test_midi_activity.cpp',
 '../../src/MidiActivity.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('SDL2'),
 link_with:libmidi,
 install:false)
test('monitor-midi-16-canales',midi_activity_test)
midi_importer_regression=executable('test-midi-importer',
 'test_midi_importer.cpp','../../src/MidiImporter.cpp',
 '../../src/PersonalFolders.cpp','../../src/FolderPlacements.cpp',
 include_directories:include_directories('../../src'),
 dependencies:[dependency('glib-2.0'),dependency('gio-2.0')],install:false)
test('importacion-midi-y-carpeta-1-fija',midi_importer_regression)

drag_page_navigation_test=executable('test-drag-page-navigation',
 'test_drag_page_navigation.cpp',
 include_directories:include_directories('../../src'),install:false)
test('arrastre-carpetas-y-midi-entre-paginas',drag_page_navigation_test)

test('inicio-portada-conservada-y-tarjetas-editables',localizacion_py,
 args:[files('test_home_info_only.py'),meson.project_source_root()],timeout:30)

test('arrastre-renombrado-no-reinicia-orden', localizacion_py,
 args:[files('test_drag_rename_no_reset.py'),meson.project_source_root()],
 timeout:30)

recents_test=executable('test-recent-songs',
 'test_recent_songs.cpp','../../src/RecentSongs.cpp',
 '../../src/AdvancedSettings.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('recientes-historial-orden-y-puntos',recents_test)

test('proteccion-anadidos-recientes-y-licencia',localizacion_py,
 args:[files('test_recent_view_protections.py'),meson.project_source_root()],timeout:30)

test('recientes-rueda-dentada-y-desplegables',localizacion_py,
 args:[files('test_recent_gear_dropdown.py'),meson.project_source_root()],timeout:30)

test('botones-silenciosos-sin-tachado-pulsacion',localizacion_py,
 args:[files('test_silent_track_buttons.py'),meson.project_source_root()],timeout:30)

test('paleta-colores-pistas-sin-ciclar',localizacion_py,
 args:[files('test_color_palette.py'),meson.project_source_root()],timeout:30)

test('dieciseis-colores-y-pulsacion-global',localizacion_py,
 args:[files('test_16_colors_global_press.py'),meson.project_source_root()],timeout:30)

test('pulsacion-sin-cuadrados-ni-marcos-externos',localizacion_py,
 args:[files('test_button_press_no_artifact.py'),meson.project_source_root()],timeout:30)

test('piano-teclas-negras-sin-verde',localizacion_py,
 args:[files('test_black_piano_keys.py'),meson.project_source_root()],timeout:30)

transport_metadata_test=executable('test-transport-metadata',
 'test_transport_metadata.cpp',link_with:libmidi,
 include_directories:include_directories('../../src'),install:false)
test('reproductor-metadata-y-bpm-real',transport_metadata_test)

test('reproductor-transporte-interactivo',localizacion_py,
 args:[files('test_transport_interactive.py'),meson.project_source_root()],timeout:30)

test('reproductor-compacto-estetica',localizacion_py,
 args:[files('test_compact_player_visual.py'),meson.project_source_root()],timeout:30)

chords_test=executable('test-chord-detector', 'test_chord_detector.cpp',
 include_directories:include_directories('../../src'),install:false)
test('acordes-reconocimiento-armonico',chords_test)
professional_meter_test=executable('test-professional-meter', 'test_professional_meter.cpp',
 link_with:libmidi,include_directories:include_directories('../../src'),install:false)
test('metrónomo-compases-cambios',professional_meter_test)

test('reproductor-display-musical-profesional',localizacion_py,
 args:[files('test_professional_player_visual.py'),meson.project_source_root()],timeout:30)

test('reproductor-tempo-rest-fallos-vista3d',localizacion_py,
 args:[files('test_tempo_fallos_perspectiva.py'),meson.project_source_root()],timeout:30)

test('reproductor-bucle-ab-interactivo',localizacion_py,
 args:[files('test_loop_ab_transporter.py'),meson.project_source_root()],timeout:30)

test('reproductor-barra-profesional-una-linea',localizacion_py,
 args:[files('test_professional_ribbon.py'),meson.project_source_root()],timeout:30)

practice_error_test=executable('test-practice-error',
 'test_practice_error.cpp',include_directories:include_directories('../../src'),
 install:false)
test('contador-solo-teclas-incorrectas',practice_error_test)

test('reproductor-inicio-pulsos-modos-2d-3d',localizacion_py,
 args:[files('test_inicio_pulsos_vistas.py'),meson.project_source_root()],timeout:30)

test('reproductor-inicio-y-vistas-click-persistencia',localizacion_py,
 args:[files('test_player_modes_click.py'),meson.project_source_root()],timeout:30)

test('reproductor-parametros-profesionales-2d-3d',localizacion_py,
 args:[files('test_player_professional_settings.py'),meson.project_source_root()],timeout:30)

test('vista-3d-perspectiva-estable',localizacion_py,
 args:[files('test_3d_geometry_clip.py'),meson.project_source_root()],timeout:30)

test('semitonos-y-notas-neon',localizacion_py,
 args:[files('test_neon_black_keys_visual.py'),meson.project_source_root()],timeout:30)

test_last_selected_song=executable('test-last-selected-song',
 'test_last_selected_song.cpp','../../src/RecentSongs.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('inicio-recuerda-ultima-cancion',test_last_selected_song)

test('capsulas-3d-neon-gradientes',localizacion_py,
 args:[files('test_3d_capsule_material.py'),meson.project_source_root()],timeout:30)

test('notas-3d-uniones-y-acabado-suave',localizacion_py,
 args:[files('test_soft_3d_finish.py'),meson.project_source_root()],timeout:30)

test('msaa-hi-dpi-uniones-subpixel',localizacion_py,
 args:[files('test_msaa_hidpi.py'),meson.project_source_root()],timeout:30)

test('teclado-3d-contacto-y-sostenido',localizacion_py,
 args:[files('test_3d_keyboard_contact.py'),meson.project_source_root()],timeout:30)

test('modo-tocar-espera-midi-y-piano-3d',localizacion_py,
 args:[files('test_mode_play_3d_keybed.py'),meson.project_source_root()],timeout:30)

test('blender3d-mallas-originales-integradas',localizacion_py,
 args:[files('test_blender_native_meshes.py'),meson.project_source_root()],timeout:30)

test('tecla-mecanica-nota-translucida-3d',localizacion_py,
 args:[files('test_press_translucent_blender.py'),meson.project_source_root()],timeout:30)

test('particulas-aditivas-y-halo-midi-3d',localizacion_py,
 args:[files('test_glow_sparks_midi.py'),meson.project_source_root()],timeout:30)

test('vista-cinematografica-comparacion-seemusic',localizacion_py,
 args:[files('test_cinematic3d_see_reference.py'),meson.project_source_root()],timeout:30)

test('llama-interna-y-liberacion-de-particulas-midi',localizacion_py,
 args:[files('test_flame_release_not_vortex.py'),meson.project_source_root()],timeout:30)

# El brillo y las partículas deben seguir activos todo el NoteOn (sin TTL).
test('efecto-llama-3d-duracion-midi',localizacion_py,
 args:[files('test_effect_duration_3d.py'),meson.project_source_root()],timeout:30)

# Verifica que el bloom y los emisores no blanqueen acordes polifónicos.
test('bloom-3d-acordes-colores-y-duracion',localizacion_py,
 args:[files('test_bloom_multinote_3d.py'),meson.project_source_root()],timeout:30)


# Behavioral coverage of continuous deterministic visual emission.
note_emitter_test=executable('test-note-emitter','test_note_emitter.cpp',
 include_directories:include_directories('../../src'),install:false)
test('particulas-3d-continuidad-temporal',note_emitter_test)


frame_pacer_test=executable('test-frame-pacer','test_frame_pacer.cpp',
 include_directories:include_directories('../../src'),install:false)
test('fotogramas-cadencia-sin-deriva',frame_pacer_test)


song_duration_test=executable('test-song-duration','test_song_duration.cpp',
 link_with:libmidi,include_directories:include_directories('../../src'),install:false)
test('midi-duracion-hasta-ultimo-noteoff',song_duration_test)

note_column_test=executable('test-note-column-sizing',
 'test_note_column_sizing.cpp',include_directories:include_directories('../../src'),
 install:false)
test('notas-3d-mas-estrechas-que-teclas',note_column_test)

track_prefs_test=executable('test-track-selection-persistence',
 'test_track_selection_persistence.cpp',
 '../../src/TrackSelectionPersistence.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('seleccion-pistas-guardada-por-cancion',track_prefs_test)

effect3d_controls_test=executable('test-effect3d-controls',
 'test_effect3d_controls.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('SDL2'),install:false)
test('efectos-3d-parametros-y-paletas',effect3d_controls_test)

birth_emitter_test=executable('test-top-birth-emitter',
 'test_top_birth_emitter.cpp',include_directories:include_directories('../../src'),
 install:false)
test('explosion-superior-particulas-descendentes',birth_emitter_test)
test('nacimiento-superior-interfaz-y-renderizado',localizacion_py,
 args:[files('test_top_birth_wiring.py'),meson.project_source_root()],timeout:30)

short_3d_visual_test=executable('test-short-3d-visual',
 'test_short_3d_visual.cpp',
 include_directories:include_directories('../../src'),install:false)
test('notas-3d-cortas-y-nacimiento-simultaneo',short_3d_visual_test)

stages_test=executable('test-stage-scenes','test_stage_scenes.cpp',
 include_directories:include_directories('../../src'),install:false)
test('escenarios-blender-animacion-camara-iluminacion',stages_test,
 args:[meson.project_source_root()])

floating_settings_test=executable('test-floating-settings',
 'test_floating_settings.cpp',include_directories:include_directories('../../src'),install:false)
test('ajustes-flotantes-limites-arrastre',floating_settings_test)
test('ajustes-flotantes-previsualizacion-sin-pausa',localizacion_py,
 args:[files('test_floating_settings_wiring.py'),meson.project_source_root()],timeout:30)

orbital_camera_test=executable('test-stage-orbit-camera',
 'test_stage_camera.cpp',include_directories:include_directories('../../src'),install:false)
test('camara-orbital-seis-perspectivas',orbital_camera_test)
test('motor-blender-geometria-3d-y-controles',localizacion_py,
 args:[files('test_stage_camera_wiring.py'),meson.project_source_root()],timeout:40)

real_finger_test=executable('test-piano-hands-midi','test_piano_hands_midi.cpp',
 include_directories:include_directories('../../src'),install:false)
test('pianista-manos-dedos-midi-teclas88',real_finger_test)

pianist_kinematic_test=executable('test-pianist-kinematics',
 'test_pianist_kinematics.cpp',include_directories:include_directories('../../src'),install:false)
test('pianista-codos-falanges-cinematica-inversa',pianist_kinematic_test)
test('modelo-humano-cc0-geometria-licencia',localizacion_py,
 args:[files('test_humano_cc0.py'),meson.project_source_root()],timeout:30)
test('pianista-piel-uv-cabeza-animada',localizacion_py,
 args:[files('test_cinematic_skin.py'),meson.project_source_root()],timeout:35)
test('pianista-acabado-profesional-3d',localizacion_py,
 args:[files('test_professional_3d_stage.py'),meson.project_source_root()],timeout:35)
test('pianista-anatomia-proporcional-y-pelo-movil',localizacion_py,
 args:[files('test_anatomia_proporcional.py'),meson.project_source_root()],timeout:40)

cinematic_camera_test=executable('test-cinematic-camera', 'test_cinematic_camera.cpp',
 include_directories:include_directories('../../src'),install:false)
test('camara-cinematica-3d-movimiento-real',cinematic_camera_test)
test('pianista-palmas-cc0-uv-camara-cinematica',localizacion_py,
 args:[files('test_cc0_palms_camera.py'),meson.project_source_root()],timeout:35)

finger_contact_test=executable('test-dedos-contacto-real',
 'test_dedos_contacto_real.cpp',
 include_directories:include_directories('../../src'),install:false)
test('pianista-yema-contacto-13-notas',finger_contact_test)

photo_hands_test=executable('test-manos-fotografia-pianista',
 'test_manos_fotografia_pianista.cpp',
 include_directories:include_directories('../../src'),install:false)
test('manos-fotografia-pulgares-acordes',photo_hands_test)

mano_rig_test=executable('test-mano-rig-16-huesos',
 'test_mano_rig_16_huesos.cpp',
 include_directories:include_directories('../../src'),install:false)
test('manos-rig-16-huesos-acordes',mano_rig_test)
test('manos-rig-malla-continua-uv',localizacion_py,
 args:[files('test_mesh_mano_rig.py'),meson.project_source_root()],timeout:40)
test('escenario-pianista-unico-teclas-midi-animadas',localizacion_py,
 args:[files('test_siete_escenarios_teclas_midi.py'),meson.project_source_root()],timeout:60)

keyboard_88_test=executable('test-88-teclas-3d',
 'test_88_teclas_escenario.cpp',
 include_directories:include_directories('../../src'),install:false)
test('escenarios-88-teclas-midi-individual',keyboard_88_test)

# Test actual MakeHuman CC0 skinned triangles against every black key's volume.
native_collision_test=executable('test-manos-colision-88',
 'test_manos_colision_88.cpp',
 include_directories:include_directories('../../src'),install:false)
test('manos-anatomicas-sin-atravesar-teclas-88',native_collision_test,
 args:[meson.project_source_root()],timeout:50)

test('cinco-nuevos-escenarios-blender-animados',localizacion_py,
 args:[files('test_cinco_musicos_blender.py'),meson.project_source_root()])

# NoteOff does not change; only the note mesh and flames have a 420ms tail.
release_envelope_test=executable('test-note-release-fade',
 'test_note_release_fade.cpp',
 include_directories:include_directories('../../src'),install:false)
test('noteoff-fade-suave-420ms',release_envelope_test)
test('noteoff-fade-pipeline-renderizado',localizacion_py,
 args:[files('test_note_release_pipeline.py'),meson.project_source_root()])

hand_practice_test=executable('test-hand-practice',
  'test_hand_practice.cpp',
  include_directories:include_directories('../../src'),install:false)
test('separacion-automatica-manos-pentagrama',hand_practice_test)
test_rmid_64bit=executable('test-rmid-64bit','test_rmid_64bit.cpp',
  include_directories:include_directories('../../src'),
  link_with:libmidi,install:false)
test('midi-rmid-64bit-debian',test_rmid_64bit)
test('manos-pentagrama-interfaz-practica',localizacion_py,
 args:[files('test_hand_practice_wiring.py'),meson.project_source_root()])
test('biblioteca-reconoce-rmi-kar-mid',localizacion_py,
 args:[files('test_midi_library_formats.py'),meson.project_source_root()])

hand_corrections_test=executable('test-hand-corrections',
 'test_hand_corrections.cpp',
 include_directories:include_directories('../../src'),install:false)
test('manos-correcciones-individuales-persistentes',hand_corrections_test)
test('manos-editor-interactivo-pentagrama',localizacion_py,
 args:[files('test_hand_editor_wiring.py'),meson.project_source_root()])

hand_ranges_finger=executable('test-hand-ranges-fingering',
  'test_hand_ranges_fingering.cpp',
  include_directories:include_directories('../../src'),install:false)
test('digitacion-1-5-tramos-ab-y-compatibilidad',hand_ranges_finger)
test('interfaz-edicion-por-tramos-y-digitacion',localizacion_py,
 args:[files('test_hand_ranges_ui.py'),meson.project_source_root()])

midi_fingering_rig=executable('test-midi-finger-guided-rig',
 'test_midi_finger_guided_rig.cpp',
 include_directories:include_directories('../../src'),install:false)
test('digitacion-midi-directa-manos-3d-huesos',midi_fingering_rig)
test('digitacion-pentagrama-rig-conectada',localizacion_py,
 args:[files('test_fingering_to_3d_pipeline.py'),meson.project_source_root()])

staff_notation_test=executable('test-staff-notation',
 'test_staff_notation.cpp',
 include_directories:include_directories('../../src'),install:false)
test('pentagramas-alteraciones-duracion-ritmica',staff_notation_test)
staff_key_meta=executable('test-staff-midi-key-signature',
 'test_staff_midi_key_meta.cpp',
 include_directories:include_directories('../../src'),
 link_with:libmidi,install:false)
test('pentagrama-tonalidad-midi-cambios',staff_key_meta)
test('pentagrama-mejoras-render-editor',localizacion_py,
 args:[files('test_staff_notation_wiring.py'),meson.project_source_root()])

engrave_model=executable('test-staff-engraving',
 'test_staff_engraving.cpp',
 include_directories:include_directories('../../src'), install:false)
test('pentagrama-polifonia-silencios-ligaduras-grupos', engrave_model)
test('pentagrama-render-editor-mismo-modelo',localizacion_py,
 args:[files('test_staff_engraving_wiring.py'),meson.project_source_root()])

musicxml_bridge=executable('test-musicxml-bridge',
 'test_musicxml_bridge.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('musicxml-extensiones-ruta-convertidor',musicxml_bridge)
test('musicxml-partituras-polifonia-tempo-articulaciones',localizacion_py,
 args:[files('test_musicxml_to_midi.py'),meson.project_source_root()])
test('musicxml-importacion-gui-zoom-persistencia',localizacion_py,
 args:[files('test_musicxml_gui_wiring.py'),meson.project_source_root()])

test('partitura-dos-sistemas-paginas-gio-inicio',localizacion_py,
 args:[files('test_score_pages_and_gio.py'),meson.project_source_root()])

practice_history_test=executable('test-practice-history',
 'test_practice_history.cpp','../../src/PracticeHistory.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('progreso-practica-historial-50-sesiones',practice_history_test)
test('progreso-practica-estadisticas-integradas',localizacion_py,
 args:[files('test_synthesia_progress_wiring.py'),meson.project_source_root()])
test('synthesia-atajos-partitura-notas-bucle',localizacion_py,
 args:[files('test_synthesia_shortcuts_wiring.py'),meson.project_source_root()])

loop_restart_test=executable('test-loop-error-policy',
 'test_loop_error_policy.cpp',
 include_directories:include_directories('../../src'),install:false)
test('synthesia-bucle-reiniciar-tras-errores',loop_restart_test)
test('synthesia-bucle-errores-integracion',localizacion_py,
 args:[files('test_loop_error_policy_wiring.py'),meson.project_source_root()])

free_play_backend=executable('test-free-play-recorder',
 'test_free_play_recorder.cpp','../../src/FreePlayRecorder.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('synthesia-modo-libre-grabador-midi-seis-pistas',free_play_backend)
test('synthesia-modo-libre-interfaz-teclado-88',localizacion_py,
 args:[files('test_free_play_wiring.py'),meson.project_source_root()])

bookmarks_test=executable('test-bookmarks',
 'test_bookmarks.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('synthesia-marcadores-musicales-persistentes',bookmarks_test)
test('synthesia-marcadores-f7-f8-f9-interfaz',localizacion_py,
 args:[files('test_bookmarks_wiring.py'),meson.project_source_root()])

metadata_test=executable('test-song-metadata',
 'test_song_metadata.cpp',
 include_directories:include_directories('../../src'),
 dependencies:dependency('glib-2.0'),install:false)
test('synthesia-canciones-metadatos-estables',metadata_test)
test('synthesia-editor-metadatos-f10-busqueda',localizacion_py,
 args:[files('test_song_metadata_wiring.py'),meson.project_source_root()])

learning_progress_test=executable('test-learning-progress',
 'test_learning_progress.cpp',
 include_directories:include_directories('../../src'),install:false)
test('synthesia-progresion-aprendizaje-0-100',learning_progress_test)

test('synthesia-luces-midi-optin-fantom-protegido',localizacion_py,
 args:[files('test_key_lights_wiring.py'),meson.project_source_root()])

free_play_visual=executable('test-free-play-visual-notes',
 'test_free_play_visual_notes.cpp',
 include_directories:include_directories('../../src'),install:false)
test('modo-libre-notas-visibles-desvanecimiento',free_play_visual)

test('modo-libre-render-mouse-midi-reproduccion',localizacion_py,
 args:[files('test_free_play_visual_wiring.py'),meson.project_source_root()])

piano_score_tracks_test=executable('test-piano-score-tracks',
 'test_piano_score_tracks.cpp',
 include_directories:include_directories('../../src'),install:false)
test('halo-multitrack-piano-partitura-no-orquesta',piano_score_tracks_test)

test('halo-render-piano-y-botones-manos',localizacion_py,
 args:[files('test_halo_focus_ui_wiring.py'),meson.project_source_root()])

test('halo-separacion-manos-colores-y-cascada-real',localizacion_py,
 args:[files('test_separacion_visible_manos.py'),meson.project_source_root()])

test('inicio-cinco-selectores-desplegables-reales',localizacion_py,
 args:[files('test_inicio_cinco_selectores.py'),meson.project_source_root()])

test('audio-usb-fantom-preamp-con-limitador-seguro',localizacion_py,
 args:[files('test_fantom_usb_boost_wiring.py'),meson.project_source_root()])

test('fantom-loudness-sf2-18db-limiter-medir-offline',localizacion_py,
 args:[files('test_fantom_loudness_calibration.py')],
 timeout:120)

full_song_test=executable('test-full-song-playback',
 'test_full_song_playback.cpp',
 include_directories:include_directories('../../src'),
 link_with:libmidi,
 dependencies:dependency('SDL2'),install:false)
test('audio-halo-modos-no-reproducir-mezcla-completa-y-velocidad-midi',
 full_song_test)
test('audio-boton-escuchar-todo-conserva-practica',localizacion_py,
 args:[files('test_full_song_wiring.py'),meson.project_source_root()])

pcm_direct_binary=executable('test-fluid-pcm-real-level',
 'test_fluid_pcm_directo.cpp','../../src/FluidEngine.cpp',
 include_directories:include_directories('../../src'),
 link_with:libmidi,
 dependencies:[dependency('fluidsynth'),dependency('libpulse-simple'),
               dependency('threads')],
 install:false)
test('fluidsynth-pcm-directo-recupera-36db-en-pipewire',localizacion_py,
 args:[files('test_fluid_pcm_directo.py'),pcm_direct_binary],
 timeout:45)

pcm_softpeak_binary=executable('test-pcm-softpeak',
 'test_pcm_softpeak.cpp',
 include_directories:include_directories('../../src'),install:false)
test('audio-pcm-salida-final-picos-limitados-sin-clipping',
 pcm_softpeak_binary)

test('selector-manos-antes-de-tocar-sin-cambiar-pistas',localizacion_py,
 args:[files('test_selector_manos_en_pistas.py'),meson.project_source_root()])

hand_auto_detector_test=executable('test-detector-manos-notas',
 'test_detector_manos_automatico.cpp',
 include_directories:include_directories('../../src'),install:false)
test('detector-manos-clasifica-solo-pistas-activas',
 hand_auto_detector_test)
test('detector-manos-boton-preparacion-partitura',
 localizacion_py,
 args:[files('test_detector_manos_en_pistas.py'),meson.project_source_root()])

test('detector-manos-dialogo-ajustes-analizar-aplicar-cancelar',
 localizacion_py,
 args:[files('test_detector_modal_settings.py'),meson.project_source_root()])

test('nuevos-botones-estilo-original-hover-pulsacion-y-estados',
 localizacion_py,
 args:[files('test_compact_button_visual_logic.py'),meson.project_source_root()])

test('exportar-video-3d-formatos-audio-restauracion-y-cancelar',
 localizacion_py,
 args:[files('test_video_export_wiring.py'),meson.project_source_root()])

musical_reasoner_test=executable('test-musical-reasoner-contextual',
 'test_musical_reasoner.cpp',
 include_directories:include_directories('../../src'),
 dependencies:[dependency('glib-2.0'),dependency('SDL2')], install:false)
test('analizador-musical-contextual-pitch-fantasma-sostenido-no-destructivo',
 musical_reasoner_test,
 env:['TEST_SONG='+meson.current_build_dir()/'test-musical-source.mid',
      'XDG_CONFIG_HOME='+meson.current_build_dir()/'musical-config'])

test('soundfont-presets-reales-por-pista-sin-midi-a-hardware',localizacion_py,
 args:[files('test_sf2_instruments_wiring.py'),meson.project_source_root()])

impact_flame_test=executable('test-impact-flame-short-rise',
 'test_impact_flame_emitter.cpp',
 include_directories:include_directories('../../src'),install:false)
test('llama-impacto-subida-corta-desvanecimiento-sin-fija',impact_flame_test)
test('escenario-sin-botones-manos-ni-grabar-video-en-ajustes',
 localizacion_py,
 args:[files('test_scene_controls_cleanup.py'),meson.project_source_root()])

test('llama-interior-movil-ascendente-contraste-sin-columna',
 localizacion_py,
 args:[files('test_inner_rising_flame.py'),meson.project_source_root()])

test('glsl-llama-compilacion-real-opengl-software',localizacion_py,
 args:[files('test_flame_runtime_glsl.py'),meson.project_source_root(),
       join_paths(meson.project_build_root(),'src/linthesia'),get_option('prefix')],
 timeout:40)


test('escenario-llama-humo-alto-47-porciento-notas-neon',
 localizacion_py,
 args:[files('test_long_flame_stage.py'),meson.project_source_root()])

test('absorcion-notas-3d-sin-cambiar-midi',
 executable('test-absorcion-notas-3d',
 'test_absorb_3d_notes.cpp',
 include_directories:include_directories('../../src'),install:false))
test('botones-video-y-absorcion-escenario-3d',localizacion_py,
 args:[files('test_absorb_video_ui.py'),meson.project_source_root()])

test('exportacion-video-carpeta-elegida-recordada-y-guardar-como',
 localizacion_py,
 args:[files('test_video_destination_choice.py'),meson.project_source_root()])

test('exportador-recupera-error-ffmpeg-sin-bloquear-reproductor',
 localizacion_py,
 args:[files('test_exporter_failure_recovery.py'),meson.project_source_root()])
