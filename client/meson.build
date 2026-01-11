client_inc = include_directories('.', '../include')

libhopper = library(
    'hopper',
    'lib.cpp',
    include_directories: client_inc,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')

pkg.generate(
    libhopper,
    name: 'hopper',
    description: 'Hopper client library',
    subdirs: 'hopper',
)

