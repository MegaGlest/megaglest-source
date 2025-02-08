shared_lib_game_deps = []

shared_lib_game_common = ['libcurl']

foreach dep : shared_lib_game_common
    shared_lib_game_deps += dependency(dep)
endforeach

subdir('shared_lib')
subdir('glest_game')
subdir('glest_map_editor')
subdir('g3d_viewer')
subdir('tools/glexemel')
