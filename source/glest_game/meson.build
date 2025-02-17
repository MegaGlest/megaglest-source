src_game = [
  'ai/ai.cpp',
  'ai/ai_interface.cpp',
  'ai/ai_rule.cpp',
  'ai/path_finder.cpp',
  'game/achievement.cpp',
  'game/chat_manager.cpp',
  'game/commander.cpp',
  'game/console.cpp',
  'game/game_camera.cpp',
  'game/game.cpp',
  'game/script_manager.cpp',
  'game/stats.cpp',
  'global/config.cpp',
  'global/core_data.cpp',
  'global/lang.cpp',
  'global/metrics.cpp',
  'graphics/particle_type.cpp',
  'graphics/renderer.cpp',
  'graphics/unit_particle_type.cpp',
  'gui/display.cpp',
  'gui/gui.cpp',
  'gui/selection.cpp',
  'main/battle_end.cpp',
  'main/intro.cpp',
  'main/main.cpp',
  'main/program.cpp',
  'menu/main_menu.cpp',
  'menu/menu_background.cpp',
  'menu/menu_state_about.cpp',
  'menu/menu_state_connected_game.cpp',
  'menu/menu_state_custom_game.cpp',
  'menu/menu_state_graphic_info.cpp',
  'menu/menu_state_join_game.cpp',
  'menu/menu_state_keysetup.cpp',
  'menu/menu_state_load_game.cpp',
  'menu/menu_state_masterserver.cpp',
  'menu/menu_state_mods.cpp',
  'menu/menu_state_new_game.cpp',
  'menu/menu_state_options.cpp',
  'menu/menu_state_options_graphics.cpp',
  'menu/menu_state_options_network.cpp',
  'menu/menu_state_options_sound.cpp',
  'menu/menu_state_root.cpp',
  'menu/menu_state_scenario.cpp',
  'menu/server_line.cpp',
  'network/client_interface.cpp',
  'network/connection_slot.cpp',
  'network/network_interface.cpp',
  'network/network_manager.cpp',
  'network/network_message.cpp',
  'network/network_protocol.cpp',
  'network/network_types.cpp',
  'network/server_interface.cpp',
  'sound/sound_container.cpp',
  'sound/sound_renderer.cpp',
  'steam/steam.cpp',
  'steamshim/steamshim_child.c',
  'type_instances/command.cpp',
  'type_instances/faction.cpp',
  'type_instances/object.cpp',
  'type_instances/resource.cpp',
  'type_instances/unit.cpp',
  'type_instances/upgrade.cpp',
  'types/command_type.cpp',
  'types/damage_multiplier.cpp',
  'types/element_type.cpp',
  'types/faction_type.cpp',
  'types/object_type.cpp',
  'types/projectile_type.cpp',
  'types/resource_type.cpp',
  'types/skill_type.cpp',
  'types/tech_tree.cpp',
  'types/tileset_model_type.cpp',
  'types/unit_type.cpp',
  'types/upgrade_type.cpp',
  'world/map.cpp',
  'world/minimap.cpp',
  'world/scenario.cpp',
  'world/surface_atlas.cpp',
  'world/tileset.cpp',
  'world/time_flow.cpp',
  'world/unit_updater.cpp',
  'world/water_effects.cpp',
  'world/world.cpp',
  'facilities/auto_test.cpp',
  'facilities/components.cpp',
  'facilities/game_util.cpp',
  'facilities/logger.cpp',
]


inc_game = [
  'types',
  'ai',
  'menu',
  'steam',
  'facilities',
  'main',
  'steamshim',
  'network',
  'world',
  'global',
  'type_instances',
  'gui',
  'sound',
  'graphics',
  'game',
]

inc_common = [
  'platform',
  'platform/posix',
  'platform/common',
  'platform/win32',
  'platform/sdl',
  'util',
  'util/utf8',
  'map',
  'compression',
  'feathery_ftp',
  'lua',
  'xml',
  'xml/rapidxml',
  'sound',
  'sound/openal',
  'graphics',
  'graphics/gl',
]

inc_ircclient = []
irclient_dep = []
ircclient_dep = dependency('ircclient', required: false)
if not ircclient_dep.found()
  ircclient_dep = cc.find_library(
    'ircclient',
    has_headers: 'libircclient.h',
    required: false,
  )
  if not ircclient_dep.found()
    inc_ircclient = ['/usr/include', '/usr/local/include']
    path = '/usr/include/libircclient'
    result = run_command('test', '-d', path, check: false)
    if result.returncode() == 0
      inc_ircclient += path
    endif
    ircclient_dep = cc.find_library(
      'ircclient',
      dirs: '/usr/local/lib',
      has_headers: 'libircclient.h',
      header_include_directories: include_directories(inc_ircclient),
    )
  endif
endif

inc_shared_lib = []
foreach dir : inc_common
  inc_shared_lib += [join_paths('..', 'shared_lib', 'include', dir)]
endforeach

dep_cppunit = dependency('cppunit')
dep_glib = dependency('glib-2.0')
dep_pthread = dependency('threads')
dep_gl = dependency('gl')
dep_x11 = dependency('x11')
# dep_ogg = dependency('ogg')
dep_vorbis = dependency('vorbisfile')
dep_glew = dependency('glew')
dep_png = dependency('libpng')
dep_jpeg = dependency('libjpeg')
dep_freetype = dependency('freetype2')

git = find_program('git')
git_version = 'unversioned'
if git.found()
  git_version = run_command(
    [git, 'rev-parse', '--short', 'HEAD'],
    capture: true,
    check: true,
  )
endif

conf.set_quoted('GITVERSION', git_version.stdout().strip())

executable(
  'megaglest',
  [src_game, '../shared_lib/sources/platform/posix/ircclient.cpp'],
  include_directories: [inc_ircclient, inc_game, inc_shared_lib],
  dependencies: [
    dep_libmegaglest,
    shared_lib_game_deps,
    lua_dep,
    ircclient_dep,
    dep_cppunit,
    dep_glib,
    dep_pthread,
    dep_x11,
    dep_vorbis,
    dep_glew,
    dep_png,
    dep_jpeg,
    dep_freetype,
  ],
)
