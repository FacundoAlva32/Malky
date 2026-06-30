project('malky', 'c',
    version: '0.0.1',
    meson_version: '>= 1.0.0',
    default_options: [
        'c_std=c17',
        'warning_level=3',
        'buildtype=debug',
    ])

gtk4 = dependency('gtk4', version: '>= 4.10')
glib = dependency('glib-2.0')
gmodule = dependency('gmodule-2.0')
m_dep = meson.get_compiler('c').find_library('m', required: true)

# Suppress GTK header pedantic warnings (G_DECLARE_FINAL_TYPE extra semicolon)
add_project_arguments('-Wno-pedantic', language: 'c')

subdir('src')
subdir('src/plugins')
