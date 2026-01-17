client_inc = include_directories('.', '../../include', '../../include/hopper/client/py')

py = import('python').find_installation('python3')

py.extension_module(
    'hopper',
    'hoppermodule.c',
    c_args: ['-Wno-unused-parameter', '-Wno-pedantic'],
    include_directories: client_inc,
    install: true,
)

