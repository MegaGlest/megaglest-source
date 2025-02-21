project('ircclient', 'c',
  meson_version : '>=0.57.0',
)

mod = import('unstable-external_project')

p = mod.add_project('configure',
  configure_options : [
                      '--prefix=@PREFIX@',
                       '--libdir=@PREFIX@/@LIBDIR@',
                       '--includedir=@PREFIX@/@INCLUDEDIR@',
                      ],
)

ircclient_dep = p.dependency('libircclient')
meson.override_dependency('libircclient', ircclient_dep)
