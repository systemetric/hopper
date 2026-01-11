project(
    'hopper',
    ['cpp'],
    version: '0.1.0',
    default_options: ['optimization=2', 'warning_level=2'],
)

inc = include_directories('include')

install_headers(
    'include/hopper/hopper.h',
    subdir: 'hopper',
)

subdir('client')
subdir('daemon')

