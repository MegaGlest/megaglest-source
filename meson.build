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
    '-fno-strict-aliasing',
    # '-Werror=strict-aliasing',
    '-DUSE_FTGL',
    '-DX11_AVAILABLE',
    '-DHAVE_SYS_IOCTL_H',
    '-DHAVE_FRIBIDI',
    # '-DHAVE_FONTCONFIG'  This didn't fix the freezing issue
    '-DUSE_STREFLOP',
    '-DSTREFLOP_SSE',
    '-DSTREFLOP_RANDOM_GEN_SIZE=32',
    '-msse',
    '-mfpmath=sse',
]

add_project_arguments(cc.get_supported_arguments(extra_flags), language: 'c')
add_project_arguments(cxx.get_supported_arguments(extra_flags), language: 'cpp')

subdir('source')
