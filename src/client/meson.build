# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR GPL-3.0-or-later

################
# Dependencies #
################

sigcpp_dep = dependency('sigc++-2.0', include_type: 'system')

##########
# Module #
##########

client_sources = files(
  'BlockModel.cpp',
  'ClientStore.cpp',
  'GraphModel.cpp',
  'ObjectModel.cpp',
  'PluginModel.cpp',
  'PluginUI.cpp',
  'PortModel.cpp',
  'ingen_client.cpp',
)

client_dependencies = [
  boost_dep,
  ingen_dep,
  lilv_dep,
  lv2_dep,
  raul_dep,
  sigcpp_dep,
  suil_dep,
]

libingen_client = shared_library(
  'ingen_client',
  client_sources,
  cpp_args: cpp_suppressions + platform_defines + ['-DINGEN_CLIENT_INTERNAL'],
  dependencies: client_dependencies,
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  include_directories: ingen_include_dirs,
  install: true,
  install_dir: ingen_module_dir,
)

ingen_client_dep = declare_dependency(
  dependencies: client_dependencies,
  link_with: libingen_client,
)
