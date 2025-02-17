# subdir('sources/util')
subdir('sources/feathery_ftp')

src = []

src_basename = [
    'string_utils.cpp',
    'util.cpp',
    'properties.cpp',
    'profiler.cpp',
    'leak_dumper.cpp',
    'checksum.cpp',
    'conversion.cpp',
    'randomgen.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/util', file)]
endforeach

src_basename = [
    'base_thread.cpp',
    'byte_order.cpp',
    'cache_manager.cpp',
    'platform_common.cpp',
    'simple_threads.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/platform/common', file)]
endforeach

src_basename = ['miniftpclient.cpp', 'miniftpserver.cpp', 'socket.cpp']

foreach file : src_basename
    src += [join_paths('sources/platform/posix', file)]
endforeach

src_basename = [
    'factory_repository.cpp',
    'gl_wrap.cpp',
    'platform_util.cpp',
    'thread.cpp',
    'window.cpp',
    'window_gl.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/platform/sdl', file)]
endforeach

src_basename = [
    'BMPReader.cpp',
    'graphics_interface.cpp',
    'quaternion.cpp',
    'buffer.cpp',
    'ImageReaders.cpp',
    'shader.cpp',
    'camera.cpp',
    'interpolation.cpp',
    'shader_manager.cpp',
    'context.cpp',
    'JPGReader.cpp',
    'texture.cpp',
    'FileReader.cpp',
    'model.cpp',
    'texture_manager.cpp',
    'font.cpp',
    'model_manager.cpp',
    'TGAReader.cpp',
    'font_manager.cpp',
    'particle.cpp',
    'video_player.cpp',
    'font_text.cpp',
    'pixmap.cpp',
    'PNGReader.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/graphics', file)]
endforeach

src_basename = [
    'base_renderer.cpp',
    'opengl.cpp',
    'context_gl.cpp',
    'particle_renderer_gl.cpp',
    'font_gl.cpp',
    'shader_gl.cpp',
    'font_textFTGL.cpp',
    'text_renderer_gl.cpp',
    'model_gl.cpp',
    'texture_gl.cpp',
    'model_renderer_gl.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/graphics/gl', file)]
endforeach

src_basename = [
    'sound_file_loader.cpp',
    'sound_player.cpp',
    'sound.cpp',
    'sound_interface.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/sound', file)]
endforeach

src_shared_lib_misc = [
    'sound/openal/sound_player_openal.cpp',
    'platform/unix/gl_wrap.cpp',
    'compression/compression_utils.cpp',
    'lua/lua_script.cpp',
    'miniz/miniz.c',
    'map/map_preview.cpp',
    'xml/xml_parser.cpp',
]

foreach file : src_shared_lib_misc
    src += [join_paths('sources', file)]
endforeach

inc_miniupnpc = ['include/platform/miniupnpc']
dep_miniupnpc = []
dep_miniupnpc = dependency('miniupnpc')
if not dep_miniupnpc.found()  # TODO: review/test for if the library isn't installed.
    dep_miniupnpc = cxx.find_library('miniupnpc', required: false)
    inc_miniupnpc += '/usr/include/miniupnpc'
endif
# if miniupnpc isn't installed, add these flags. Check existing cmake in
# shared_lib to see about other platforms
#c_args: ['-DMINIUPNPC_SET_SOCKET_TIMEOUT', '-D_BSD_SOURCE'],
#  cpp_args: ['-DMINIUPNPC_SET_SOCKET_TIMEOUT', '-D_BSD_SOURCE'],

inc_shared_lib_misc_basename = [
    'graphics',
    'map',
    'xml',
    'sound',
    'sound/openal',
    'platform/common',
    'platform/posix',
    'platform/sdl',
    'graphics',
    'graphics/gl',
    'util/',
    'util/utf8',
    'compression',
    'lua',
]

inc_shared_lib_misc = []

foreach file : inc_shared_lib_misc_basename
    inc_shared_lib_misc += [join_paths('include', file)]
endforeach

# Note on includes
#
# You should not do that but use relative paths instead, for
# directories that are part of your project.
#
# To get include path to any directory relative to the current dir do
#
# incdir = include_directories(dirname)
#
# After this incdir will contain both the current source dir as well as the
# corresponding build dir. It can then be used in any subdirectory and
# Meson will take care of all the busywork to make paths work.
#
# Dirname can even be '.' to mark the current directory. Though you should
# remember that the current source and build directories are always
# put in the include directories by default so you only need to do
# include_directories('.') if you intend to use the result in a
# different subdirectory.
lib_megaglest = static_library(
    'megaglest',
    src,
    include_directories: [
        include_directories('../..'), # for config.h
        inc_shared_lib_misc,
        'sources',  # for miniz
        inc_miniupnpc,
    ],
    dependencies: [
        streflop_dep,
        shared_lib_game_deps,
        dep_feathery_ftp,
        dep_miniupnpc,
        sdl_dep,
    ],
    pic: true,
    cpp_args: ['-DSTREFLOP_SSE'],
)

dep_libmegaglest = declare_dependency(
    link_with: lib_megaglest,
    dependencies: [sdl_dep, streflop_dep],
    include_directories: include_directories('../..'), # for config.h
)
