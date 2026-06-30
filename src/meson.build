malky_sources = files(
    'main.c',
    'core/app.c',
    'core/event.c',
    'core/config.c',
    'core/window.c',
    'core/movement.c',
    'core/dialog.c',
    'core/pet.c',
    'core/skin.c',
    'core/dragdrop.c',
    'core/plugin.c',
    'core/installer.c',
    'core/context_menu.c',
)

malky_incs = [
    include_directories('core'),
    include_directories('plugins/sdk'),
]

executable('malky',
    malky_sources,
    include_directories: malky_incs,
    dependencies: [gtk4, glib, gmodule, m_dep],
    install: true,
)
