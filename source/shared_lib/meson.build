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

dep_sdl = dependency('sdl2')
dep_openal = dependency('openal')
dep_ftgl = dependency('ftgl')

# This will get changed to 5.3
lua_dep = dependency('lua-5.3', required: true)
if not lua_dep.found()
    lua_dep = cc.find_library('lua', has_headers: 'lua.h', required: true)
    #  if not lua_dep.found()
    #    # This will use the fallback
    #    lua_dep = dependency(
    #      'lua-5.3',
    #      required: true,
    #      fallback: 'lua',
    #      default_options: ['default_library=static']
    #      )
    #  endif
endif

# By default, meson will look in the repo roots 'subproject' directory # Note
# changing a default options might not be enough to trigger meson to rerun
# setup. In the build directory, you can change the option manually
# Example: meson configure -Dstreflop:fpu=soft (where the value preceding the
# colon is the subproject name) and then rebuild.
streflop_subproj = subproject(
    'streflop',
    default_options: ['default_library=static', 'fpu=sse', 'no_denormals=true'],
)
# TODO: The binaries don't need the dep object, but they do need the includes.
# Get them from the dep object instead of hard-coding them in the three
# binary meson builds.
streflop_dep = streflop_subproj.get_variable('streflop_dep')
streflop_includes = streflop_dep.partial_dependency(includes: true)

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

lib_megaglest = static_library(
    'megaglest',
    src,
    include_directories: [
        inc_shared_lib_misc,
        'sources',  # for miniz
        inc_miniupnpc,
    ],
    dependencies: [
        streflop_dep,
        shared_lib_game_deps,
        lua_dep,
        dep_sdl,
        dep_openal,
        dep_feathery_ftp,
        dep_miniupnpc,
        dep_ftgl,
    ],
    pic: true,
)

dep_libmegaglest = declare_dependency(
    link_with: lib_megaglest,
    dependencies: streflop_includes,
)
