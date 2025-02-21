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

game_deps = []

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
#    path = '/usr/include/libircclient' # location on Arch-based systems
#    result = run_command('test', '-d', path, check: false)
#    if result.returncode() == 0
#      inc_ircclient += path
#    endif
    ircclient_dep = cc.find_library(
      'ircclient',
      dirs: '/usr/local/lib',
      has_headers: 'libircclient.h',
      header_include_directories: include_directories(inc_ircclient),
      required: false,
    )
    if not ircclient_dep.found()
      # This will use the fallback
      ircclient_dep = dependency(
        'ircclient',
        required: true,
        fallback: 'ircclient',
        default_options: ['default_library=static']
        )
    endif
  endif
endif

game_deps += ircclient_dep

game_deps_required = [
  'cppunit',
  'glib-2.0',
  'threads',
  'vorbisfile',
  'freetype2',
]

foreach dep : game_deps_required
  game_deps += dependency(dep)
endforeach

inc_miniupnpc = ['../shared_lib/include/platform/miniupnpc']
miniupnpc_dep = []
miniupnpc_dep = dependency('miniupnpc')
if not miniupnpc_dep.found()  # TODO: review/test for if the library isn't installed.
  miniupnpc_dep = cxx.find_library('miniupnpc', required: false)
  inc_miniupnpc += '/usr/include/miniupnpc'
endif
game_deps += miniupnpc_dep
# if miniupnpc isn't installed, add these flags. Check existing cmake in
# shared_lib to see about other platforms
#c_args: ['-DMINIUPNPC_SET_SOCKET_TIMEOUT', '-D_BSD_SOURCE'],
#  cpp_args: ['-DMINIUPNPC_SET_SOCKET_TIMEOUT', '-D_BSD_SOURCE'],

src_net_basename = [
  'miniftpclient.cpp',
  'miniftpserver.cpp',
  'socket.cpp',
  'ircclient.cpp',
]

foreach file : src_net_basename
  src_game += [join_paths('../shared_lib/sources/platform/posix', file)]
endforeach

executable(
  'megaglest',
  [src_game],
  include_directories: [inc_ircclient, inc_game, inc_shared_lib, inc_miniupnpc],
  dependencies: [
    dep_libmegaglest,
    shared_lib_game_deps,
    gl_dep,
    game_deps,
    lua_dep,
    jpeg_dep,
    png_dep,
    glew_dep,
    feathery_ftp_dep,
    x11_dep,
  ],
)
