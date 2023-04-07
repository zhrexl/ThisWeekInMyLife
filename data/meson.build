desktop_file = i18n.merge_file(
        input: 'io.github.zhrexl.ThisWeekInMyLife.desktop.in',
       output: 'io.github.zhrexl.ThisWeekInMyLife.desktop',
         type: 'desktop',
       po_dir: '../po',
      install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils, args: [desktop_file])
endif


metainfo = i18n.merge_file(
  input: 'io.github.zhrexl.ThisWeekInMyLife.appdata.xml.in',
  output: 'io.github.zhrexl.ThisWeekInMyLife.metainfo.xml',
  install: true,
  install_dir: join_paths(join_paths('share'), 'metainfo'),
  po_dir: join_paths(meson.project_source_root(), 'po')
)


appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util, args: ['validate', metainfo])
endif

install_data('io.github.zhrexl.ThisWeekInMyLife.gschema.xml',
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file',
       compile_schemas,
       args: ['--strict', '--dry-run', meson.current_source_dir()])
endif

subdir('icons')
