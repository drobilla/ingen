# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

sources = files(
  'AtomForge.cpp',
  'AtomReader.cpp',
  'AtomWriter.cpp',
  'ClashAvoider.cpp',
  'ColorContext.cpp',
  'Configuration.cpp',
  'Forge.cpp',
  'LV2Features.cpp',
  'Library.cpp',
  'Log.cpp',
  'Parser.cpp',
  'Resource.cpp',
  'Serialiser.cpp',
  'Store.cpp',
  'StreamWriter.cpp',
  'TurtleWriter.cpp',
  'URI.cpp',
  'URIMap.cpp',
  'URIs.cpp',
  'World.cpp',
  'runtime_paths.cpp',
)

if have_socket
  sources += files('SocketReader.cpp', 'SocketWriter.cpp')
endif

ingen_deps = [
  boost_dep,
  lilv_dep,
  lv2_dep,
  raul_dep,
  serd_dep,
  sord_dep,
  sratom_dep,
  thread_dep,
]

ingen_include_dirs = include_directories('../include', 'include')

libingen = shared_library(
  versioned_name,
  sources,
  cpp_args: cpp_suppressions + platform_defines,
  darwin_versions: [major_version + '.0.0', meson.project_version()],
  dependencies: ingen_deps,
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  include_directories: ingen_include_dirs,
  install: true,
  soversion: soversion,
  version: meson.project_version(),
)

ingen_dep = declare_dependency(
  dependencies: ingen_deps,
  include_directories: include_directories('../include'),
  link_with: libingen,
)

subdir('server')
subdir('client')
subdir('gui')
