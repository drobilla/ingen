# Copyright 2019-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

#####################
# Integration Tests #
#####################

ingen_test = executable(
  'ingen_test',
  files('ingen_test.cpp'),
  cpp_args: cpp_suppressions + platform_defines,
  dependencies: [ingen_dep],
)

ingen_bench = executable(
  'ingen_bench',
  files('ingen_bench.cpp'),
  cpp_args: cpp_suppressions + platform_defines,
  dependencies: [ingen_dep],
)

empty_manifest = files('empty.ingen/manifest.ttl')
empty_main = files('empty.ingen/main.ttl')

integration_tests = [
  'connect_disconnect_node_node',
  'connect_disconnect_node_patch',
  'connect_disconnect_patch_patch',
  'copy_node',
  'create_delete_node',
  'create_delete_patch',
  'create_delete_poly_patch',
  'create_delete_port',
  'disconnect_all_node',
  'disconnect_all_port',
  'duplicate_node',
  'enable_graph',
  'get_engine',
  'get_node',
  'get_patch',
  'get_plugin',
  'get_plugins',
  'get_port',
  'load_graph',
  'move_node',
  'move_port',
  'move_root_port',
  'poly',
  'put_audio_in',
  'save_graph',
  'set_graph_poly',
  'set_patch_port_value',
]

test_env = environment(
  {
    'INGEN_MODULE_PATH': ':'.join(
      [
        ingen_build_root / 'src',
        ingen_build_root / 'src' / 'client',
        ingen_build_root / 'src' / 'gui',
        ingen_build_root / 'src' / 'server',
      ],
    ),
    'LV2_PATH': ':'.join(
      [
        lv2_dep.get_variable(
          default_value: lv2dir,
          internal: 'lv2dir',
          pkgconfig: 'lv2dir',
        ),
        lv2_dep.get_variable(
          default_value: lv2dir,
          internal: 'plugindir',
          pkgconfig: 'plugindir',
        ),
      ],
    ),
  },
)

foreach test : integration_tests
  test(
    test,
    ingen_test,
    env: test_env,
    args: [
      ['--load', empty_manifest],
      ['--execute', files(test + '.ttl')],
    ],
  )
endforeach

########
# Lint #
########

if get_option('lint')
  subdir('lint')
endif
