subdir('sources/streflop')

src_basename = [
  'base_thread.cpp',
  'byte_order.cpp',
  'cache_manager.cpp',
  'platform_common.cpp',
  'simple_threads.cpp',
]

src = []
foreach file : src_basename
    src += [join_paths('sources/platform/common/', file)]
endforeach

dep_curl = dependency('libcurl')
dep_sdl = dependency('sdl2')

# lib_megaglest = static_library(
#   'megaglest',
#   src,
#   include_directories : [
#     'include/streflop',
#     'include/platform/common',
#     'include/platform/posix',
#     'include/platform/sdl',
#     'include/util',
#     'include/graphics',
#   ],
#   dependencies: [dep_curl, dep_sdl, dep_streflop]
#   # dependencies: [dep_curl, dep_sdl, dep_streflop]
#   )

# dep_rmw = declare_dependency(link_with: lib_rmw, include_directories : '..')
