client_inc = include_directories('.', '../../include')

py = import('python').find_installation('python3')

py.extension_module(
    'hopper',
    'hoppermodule.c',
    include_directories: client_inc,
    install: true,
)

