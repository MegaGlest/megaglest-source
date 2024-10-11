subdir('sources/streflop')
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

src_basename = [
  'ircclient.cpp',
  'miniftpclient.cpp',
  'miniftpserver.cpp',
  'socket.cpp',
]

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
  'xml/xml_parser.cpp'
]

foreach file : src_shared_lib_misc
    src += [join_paths('sources', file)]
endforeach

dep_curl = dependency('libcurl')
dep_sdl = dependency('sdl2')
dep_openal = dependency('openal')
dep_ftgl = dependency('ftgl')

# Using simply 'lua' doesn't work on all Linux distributions
# See https://github.com/mesonbuild/meson/issues/4553 for more info
foreach name : ['lua', 'lua5.3', 'lua-5.3', 'lua53']
    dep_lua = dependency(name, version: '>=5.3', required: false)
    if dep_lua.found()
        break
    endif
endforeach

if not dep_lua.found()
    error('Lua could not be found!')
endif

inc_ircclient = []
dep_irclient = []
dep_ircclient = dependency('libircclient', required: false)
if not dep_ircclient.found()
  dep_ircclient = cc.find_library('libircclient', has_headers: 'libircclient.h', required: false)
  if not dep_ircclient.found()
    inc_ircclient = '/usr/include/libircclient'
    dep_ircclient = cc.find_library(
      'libircclient',
      has_headers: 'libircclient.h',
      header_include_directories: include_directories(inc_ircclient)
    )
  endif
endif

inc_miniupnpc = ['include/platform/miniupnpc']
dep_miniupnpc = []
dep_miniupnpc = dependency('miniupnpc')
if not dep_miniupnpc.found() # TODO: review/test for if the library isn't installed.
  dep_miniupnpc = cxx.find_library('miniupnpc', required: false)
  inc_miniupnpc += '/usr/include/miniupnpc'
endif

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
  include_directories : [
    inc_shared_lib_misc,
    'sources', # for miniz
    inc_ircclient,
    inc_miniupnpc,
  ],
  dependencies: [
    dep_curl,
    dep_sdl,
    dep_openal,
    dep_lua,
    dep_feathery_ftp,
    dep_ircclient,
    dep_miniupnpc,
    dep_ftgl,
    dep_streflop
  ]
  )

dep_libmegaglest = declare_dependency(link_with: lib_megaglest)
