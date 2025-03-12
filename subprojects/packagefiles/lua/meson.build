project(
  'lua-5.1', 'c',
  license: 'MIT',
  meson_version: '>=0.49.2',
  version: '5.1.5',
)

lua_versions = meson.project_version().split('.')
# Override the c_std and warning_level of any superproject.
project_options = ['c_std=c99', 'warning_level=2']
cc = meson.get_compiler('c')

# Skip bogus warning.
if cc.get_id() == 'clang'
  add_project_arguments('-Wno-string-plus-int', language: 'c')
elif cc.get_id() == 'gcc'
  add_project_arguments('-Wno-stringop-overflow', language: 'c')
endif

add_project_arguments('-D_XOPEN_SOURCE=600', language: 'c')

# Platform-specific defines.
is_posix = host_machine.system() not in ['windows', 'emscripten', 'android']
if is_posix
  add_project_arguments('-DLUA_USE_POSIX', language: 'c')
endif

# Library dependencies.
lua_lib_deps = [cc.find_library('m', required: false)]

if get_option('loadlib')
  dl_dep = cc.find_library('dl', required: false)
  lua_lib_deps += [dl_dep]
  add_project_arguments('-DLUA_USE_DLOPEN', language: 'c')
endif

# Interpreter dependencies.
lua_exe_deps = []
lua_exe_args = []

if get_option('line_editing')
  readline_dep = dependency('readline', required: false)
  if not readline_dep.found()
    readline_dep = dependency('libedit', required: false)
    if not readline_dep.found()
      error('''Didn't find readline or libedit, can't enable line editing.''')
    endif
  endif
  lua_exe_deps += [readline_dep]
  lua_exe_args += ['-DLUA_USE_READLINE']
endif

# Targets.
lua_static = static_library('_lua',
  'src/print.c',
  'src/lstring.c',
  'src/ltablib.c',
  'src/loadlib.c',
  'src/lmathlib.c',
  'src/lstate.c',
  'src/ldump.c',
  'src/loslib.c',
  'src/liolib.c',
  'src/ldblib.c',
  'src/lbaselib.c',
  'src/lapi.c',
  'src/llex.c',
  'src/lopcodes.c',
  'src/lvm.c',
  'src/ltable.c',
  'src/linit.c',
  'src/lobject.c',
  'src/lstrlib.c',
  'src/lcode.c',
  'src/ldebug.c',
  'src/lauxlib.c',
  'src/ldo.c',
  'src/lfunc.c',
  'src/lzio.c',
  'src/ltm.c',
  'src/lgc.c',
  'src/lmem.c',
  'src/lparser.c',
  'src/lundump.c',

  dependencies: lua_lib_deps,
  override_options: project_options,
  implicit_include_directories: false,
)

lua_lib = library('lua',
  link_whole: lua_static,
  dependencies: lua_lib_deps,
  version: meson.project_version(),
  soversion: lua_versions[0] + '.' + lua_versions[1],
  override_options: project_options,
  implicit_include_directories: false,
)

if cc.get_id() == 'msvc'
  # Lua does not export symbols
  # so we have to compile executables
  # statically
  lua_link = lua_static
else
  lua_link = lua_lib
endif

if get_option('interpreter')
  lua_exe = executable('lua',
    'src/lua.c',
    c_args: lua_exe_args,
    link_with: lua_link,
    dependencies: lua_exe_deps,
    export_dynamic: get_option('loadlib'),
    override_options: project_options,
    implicit_include_directories: false,
  )

  if get_option('default_library') == 'static'
    luac_exe = executable('luac',
      'src/luac.c',
      link_with: lua_link,
      override_options: project_options,
      implicit_include_directories: false,
    )
  endif
endif

inc = include_directories('src')
lua_dep = declare_dependency(link_with: lua_lib, include_directories: inc)
