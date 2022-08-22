# subdir('sources/streflop')
subdir('sources/feathery_ftp')
# subdir('sources/util')

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

inc_util = [

]

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

dep_curl = dependency('libcurl')
dep_sdl = dependency('sdl2')
dep_x11 = dependency('x11')
dep_openal = dependency('openal')
dep_lua = dependency('lua')
dep_cppunit = dependency('cppunit')
dep_fribidi = dependency('fribidi')
dep_glib = dependency('glib-2.0')
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
    'include/streflop',
    'include/platform/common',
    'include/platform/posix',
    'include/platform/sdl',
    'include/graphics',
    'include/util/',
    'include/util/utf8',
  ],
  dependencies: [
	dep_curl,
	dep_sdl, dep_x11, dep_openal, dep_lua,
	dep_cppunit,
	dep_fribidi,
	dep_glib
	]
  )

# dep_libmegaglest = declare_dependency(link_with: lib_megaglest, include_directories : '..')
