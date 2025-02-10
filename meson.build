project(
    'MegaGlest',
    ['c', 'cpp'],
    version: '3.13.0.999',
    meson_version: '>= 0.55.0',
    default_options: [
        'warning_level=1',
        'buildtype=debugoptimized',
        'cpp_std=c++11',
    ],
)

# Setup compiler.
cxx = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

extra_flags = [
    '-fno-common',
    '-Werror=odr',
    '-Werror=lto-type-mismatch',
    # '-Werror=strict-aliasing',
    '-DUSE_FTGL',
    '-DUSE_STREFLOP',
    '-DSTREFLOP_SSE',
]

add_project_arguments(cc.get_supported_arguments(extra_flags), language: 'c')
add_project_arguments(cxx.get_supported_arguments(extra_flags), language: 'cpp')

subdir('source')
