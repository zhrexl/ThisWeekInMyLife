kanban_sources = [
  'main.c',
  'kanban-application.c',
  'kanban-window.c',
  'kanban-card.c',
  'kanban-column.c',
]

kanban_deps = [
  dependency('gtk4'),
  dependency('json-glib-1.0', version: '>= 1.0'),
  dependency('libadwaita-1', version: '>= 1.2'),
]

subdir('utils')

kanban_sources += gnome.compile_resources('thisweekinmylife-resources',
  'thisweekinmylife.gresource.xml',
  c_name: 'thisweekinmylife'
)

executable('thisweekinmylife', kanban_sources,
  dependencies: kanban_deps,
       install: true,
)
