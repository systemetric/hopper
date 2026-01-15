client_inc = include_directories('.', '../include')

libhopper = static_library(
    'hopper',
    'lib.c',
    include_directories: client_inc,
    pic: true,
    install: true,
)

pkg = import('pkgconfig')

pkg.generate(
    libhopper,
    name: 'hopper',
    description: 'Hopper client library',
    version: meson.project_version(),
    subdirs: 'hopper',
)

