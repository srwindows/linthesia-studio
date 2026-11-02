subdir('libmidi')

conf_data = configuration_data()
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config = configure_file(output : 'config.h',
                        configuration : conf_data)

dependencies = [
  dependency('alsa'),
  dependency('gl'),
  dependency('giomm-2.4'),
  dependency('sqlite3'),
  dependency('SDL2'),
  dependency('SDL2_ttf'),
  dependency('SDL2_image'),
  dependency('threads'),
  dependency('fluidsynth'),
  dependency('libpulse-simple')
]

executable('linthesia',
           config,
           'MenuLayout.cpp',
           'CompatibleSystem.cpp',
           'StringTile.cpp',
           'TitleState.cpp',
           'AdvancedSettings.cpp',
           'UiAppearance.cpp',
           'MidiActivity.cpp',
           'SettingsPanel.cpp',
           'FluidEngine.cpp',
           'Tga.cpp',
           'SongTile.cpp',
           'TrackTile.cpp',
           'UserSettings.cpp',
           'TextWriter.cpp',
           'EnumTile.cpp',
           'PlayingState.cpp',
           'VideoExporter.cpp',
           'PlayingVideoExport.cpp',
           'RecentSongs.cpp',
           'PracticeHistory.cpp',
           'FreePlayRecorder.cpp',
           'FreePlayState.cpp',
           'KeyboardDisplay.cpp',
           'TrackSelectionState.cpp',
           'TrackSelectionPersistence.cpp',
           'GameState.cpp',
           'Keyboard.cpp',
           'MidiComm.cpp',
           'Renderer.cpp',
           'LiveStage3D.cpp',
           'DeviceTile.cpp',
           'FrameAverage.cpp',
           'StatsState.cpp',
           'LinthesiaError.cpp',
           'main.cpp',
           'SongLibState.cpp',
           'PersonalFolders.cpp',
           'FolderPlacements.cpp',
           'MidiSearchIndex.cpp',
           'MidiImporter.cpp',
           'DpmsThread.cpp',
           'FileSelector.cpp',
           dependencies : dependencies,
           link_with: libmidi,
           # Direct launches must find the matching MIDI library. The desktop
           # launcher also prioritizes this same location.
           build_rpath : '$ORIGIN/../lib/x86_64-linux-gnu',
           install_rpath : '$ORIGIN/../lib/x86_64-linux-gnu',
           install : true)

install_data('midi_import_picker.py', install_dir: get_option('datadir') / 'linthesia' / 'scripts')
install_data('../tools/musicxml_to_midi.py', install_dir: get_option('datadir') / 'linthesia' / 'scripts')
