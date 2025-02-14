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
    '-DHAVE_SYS_IOCTL_H',
    '-DSTREFLOP_SSE',
    '-DSTREFLOP_RANDOM_GEN_SIZE=32',
    '-msse',
    '-mfpmath=sse',
]

# Defined in cmake but I didn't find references in the source code
# '-DX11_AVAILABLE'
# -DHAVE_FONTCONFIG'

add_project_arguments(cc.get_supported_arguments(extra_flags), language: 'c')
add_project_arguments(cxx.get_supported_arguments(extra_flags), language: 'cpp')

# Macros can be written to config.h (see below) using conf.set()
conf = configuration_data()

subdir('source')

# The config.h file finally is output here, all the data assigned by
# conf.set() will be written to it.
config_h = configure_file(output : 'config_whole.h', configuration : conf)
