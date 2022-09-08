# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

##########
# Module #
##########

server_sources = files(
  'ArcImpl.cpp',
  'BlockFactory.cpp',
  'BlockImpl.cpp',
  'Broadcaster.cpp',
  'Buffer.cpp',
  'BufferFactory.cpp',
  'ClientUpdate.cpp',
  'CompiledGraph.cpp',
  'ControlBindings.cpp',
  'DuplexPort.cpp',
  'Engine.cpp',
  'EventWriter.cpp',
  'GraphImpl.cpp',
  'InputPort.cpp',
  'InternalBlock.cpp',
  'InternalPlugin.cpp',
  'LV2Block.cpp',
  'LV2Plugin.cpp',
  'NodeImpl.cpp',
  'PortImpl.cpp',
  'PostProcessor.cpp',
  'PreProcessor.cpp',
  'RunContext.cpp',
  'SocketListener.cpp',
  'Task.cpp',
  'UndoStack.cpp',
  'Worker.cpp',
  'events/Connect.cpp',
  'events/Copy.cpp',
  'events/CreateBlock.cpp',
  'events/CreateGraph.cpp',
  'events/CreatePort.cpp',
  'events/Delete.cpp',
  'events/Delta.cpp',
  'events/Disconnect.cpp',
  'events/DisconnectAll.cpp',
  'events/Get.cpp',
  'events/Mark.cpp',
  'events/Move.cpp',
  'events/SetPortValue.cpp',
  'events/Undo.cpp',
  'ingen_engine.cpp',
  'internals/BlockDelay.cpp',
  'internals/Controller.cpp',
  'internals/Note.cpp',
  'internals/Time.cpp',
  'internals/Trigger.cpp',
  'mix.cpp',
)

server_dependencies = [
  boost_dep,
  ingen_dep,
  lilv_dep,
  raul_dep,
  serd_dep,
  sord_dep,
  sratom_dep,
  thread_dep,
]

server_include_dirs = include_directories(
  '.',
  '../../include',
  '../include',
)

libingen_server = shared_library(
  'ingen_server',
  server_sources,
  cpp_args: cpp_suppressions + platform_defines + ['-DINGEN_SERVER_INTERNAL'],
  dependencies: server_dependencies,
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  include_directories: server_include_dirs,
  install: true,
  install_dir: ingen_module_dir,
)

ingen_server_dep = declare_dependency(
  dependencies: server_dependencies,
  link_with: libingen_server,
)

###########
# Drivers #
###########

if jack_dep.found()
  shared_module(
    'ingen_jack',
    files('JackDriver.cpp', 'ingen_jack.cpp'),
    cpp_args: cpp_suppressions + platform_defines,
    dependencies: [ingen_server_dep, jack_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: ingen_include_dirs,
    install: true,
    install_dir: ingen_module_dir,
  )
endif

if portaudio_dep.found()
  shared_module(
    'ingen_portaudio',
    files('PortAudioDriver.cpp', 'ingen_portaudio.cpp'),
    cpp_args: cpp_suppressions + platform_defines,
    dependencies: [ingen_server_dep, portaudio_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: ingen_include_dirs,
    install: true,
    install_dir: ingen_module_dir,
  )
endif

shared_module(
  'ingen_lv2',
  files('ingen_lv2.cpp'),
  cpp_args: cpp_suppressions + platform_defines,
  dependencies: [ingen_server_dep, lv2_dep],
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  include_directories: ingen_include_dirs,
  install: true,
  install_dir: lv2dir / 'ingen.lv2',
)
