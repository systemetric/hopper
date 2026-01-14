project(
    'hopper',
    ['cpp'],
    version: '0.1.0',
    default_options: ['optimization=0', 'warning_level=3', 'cpp_std=c++20'],
)


inc = include_directories('include')

install_headers(
    'include/hopper/hopper.h',
    subdir: 'hopper',
)

subdir('client')
subdir('daemon')

