# subdir('sources/streflop')
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
    src += [join_paths('sources/util/', file)]
endforeach

src_basename = [
  'base_thread.cpp',
  'byte_order.cpp',
  'cache_manager.cpp',
  'platform_common.cpp',
  'simple_threads.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/platform/common/', file)]
endforeach

src_basename = [
  'ircclient.cpp',
  'miniftpclient.cpp',
  'miniftpserver.cpp',
  'socket.cpp',
]

foreach file : src_basename
    src += [join_paths('sources/platform/posix/', file)]
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
    src += [join_paths('sources/platform/sdl/', file)]
endforeach

dep_curl = dependency('libcurl')
dep_sdl = dependency('sdl2')
dep_x11 = dependency('x11')
dep_openal = dependency('openal')

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

dep_cppunit = dependency('cppunit')
dep_fribidi = dependency('fribidi')
dep_glib = dependency('glib-2.0')
# TODO: needs condition for OSes which can/ can't find libircclient.
# dep_ircclient = dependency('libircclient')
inc_ircclient = '/usr/include/libircclient'
dep_ircclient = cxx.find_library(
  'libircclient', 
  has_headers: 'libircclient.h',
  header_include_directories: include_directories(inc_ircclient),
  )

inc_miniupnpc = ['/usr/include/miniupnpc', 'include/platform/miniupnpc']
dep_ircclient = cxx.find_library(
  'miniupnpc', 
  has_headers: 'miniupnpc.h',
  header_include_directories: include_directories(inc_miniupnpc),
  )
dep_miniupnpc = dependency('miniupnpc')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')
#dep_ = dependency('')

#-lcppunit
#-lfribidi
#-lglib-2.0
#-lGL
#-lGLU
#-lSM
#-lICE
#-lXext
#-lopenal
#-llua5.3
#-lm #£ math lib
#-ldl
#-ljpeg
#-lpng
#-lfontconfig
#-lftgl
#-lfreetype
#-lGLEW
#-lminiupnpc
#-lircclient
#-lSDL2main
#-lfribidi
#-lglib-2.0

lib_megaglest = static_library(
  'megaglest',
  src,
  include_directories : [
    'include/graphics',
    'include/map',
    'include/xml',
    'include/sound',
    'include/sound/openal',
    'include/platform/common',
    'include/platform/posix',
    'include/platform/sdl',
    'include/graphics',
    'include/graphics/gl',
    'include/util/',
    'include/util/utf8',
    inc_ircclient,
    inc_miniupnpc,
  ],
  dependencies: [
	dep_curl,
	dep_sdl,
  dep_x11,
  dep_openal,
  dep_lua,
	dep_cppunit,
	dep_fribidi,
	dep_glib,
  dep_feathery_ftp,
  dep_ircclient,
  dep_miniupnpc,
	]
  )

# dep_libmegaglest = declare_dependency(link_with: lib_megaglest, include_directories : '..')
