project(
  'yolodex',
  'cpp',
  version: '0.1.10',
  license: 'Unlicense',
  meson_version: '>=0.56',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'prefix=/usr',
  ],
)

gtkmm = dependency('gtkmm-3.0')
libxml = dependency('libxml-2.0')
fontconfig = dependency('fontconfig')

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('APP_ID', 'org.gmgauthier.Yolodex')
conf.set_quoted('DATADIR', get_option('prefix') / get_option('datadir') / 'yolodex')
conf.set_quoted('SOURCE_ROOT', meson.project_source_root())

configure_file(
  output: 'config.hpp',
  configuration: conf,
)

inc = include_directories('.', 'src')

executable(
  'yolodex',
  'src/main.cpp',
  'src/application.cpp',
  'src/paths.cpp',
  'src/main_window.cpp',
  'src/about_dialog.cpp',
  'src/card_face.cpp',
  'src/card_view.cpp',
  'src/stack.cpp',
  'src/save_path.cpp',
  'src/print_layout.cpp',
  'src/find.cpp',
  'src/settings.cpp',
  'src/find_dialog.cpp',
  'src/font_dialog.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml, fontconfig],
  install: true,
  install_dir: get_option('bindir'),
)

install_data(
  'data/yolodex.desktop',
  install_dir: get_option('datadir') / 'applications',
)

install_data(
  'data/skin/lcos/lcos.css',
  install_dir: get_option('datadir') / 'yolodex' / 'skin' / 'lcos',
)

install_data(
  'brand/icon-tile.svg',
  install_dir: get_option('datadir') / 'yolodex' / 'brand',
)

install_data(
  'brand/icon-tile.svg',
  rename: 'yolodex.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

yolodex_tests = executable(
  'yolodex-tests',
  'tests/test_stack.cpp',
  'src/stack.cpp',
  include_directories: inc,
  dependencies: [gtkmm, libxml],
)
test('stack', yolodex_tests)

save_path_tests = executable(
  'yolodex-save-path-tests',
  'tests/test_save_path.cpp',
  'src/save_path.cpp',
  include_directories: inc,
)
test('save_path', save_path_tests)

print_tests = executable(
  'yolodex-print-tests',
  'tests/test_print.cpp',
  'src/print_layout.cpp',
  include_directories: inc,
)
test('print', print_tests)

find_tests = executable(
  'yolodex-find-tests',
  'tests/test_find.cpp',
  'src/find.cpp',
  include_directories: inc,
  dependencies: [gtkmm],
)
test('find', find_tests)

card_face_tests = executable(
  'yolodex-card-face-tests',
  'tests/test_card_face.cpp',
  'src/card_face.cpp',
  include_directories: inc,
  dependencies: [gtkmm],
)
test('card_face', card_face_tests)

settings_tests = executable(
  'yolodex-settings-tests',
  'tests/test_settings.cpp',
  'src/settings.cpp',
  include_directories: inc,
  dependencies: [gtkmm],
)
test('settings', settings_tests)
