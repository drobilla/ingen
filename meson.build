# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

project(
  'ingen',
  'cpp',
  default_options: [
    'b_ndebug=if-release',
    'buildtype=release',
    'cpp_std=c++17',
  ],
  license: 'GPLv3+',
  meson_version: '>= 0.56.0',
  version: '0.5.1',
)

ingen_src_root = meson.current_source_dir()
ingen_build_root = meson.current_build_dir()
major_version = meson.project_version().split('.')[0]
versioned_name = '@0@-@1@'.format(meson.project_name(), major_version)

#######################
# Compilers and Flags #
#######################

# Required tools
cpp = meson.get_compiler('cpp')

# Set global warning suppressions
subdir('meson/suppressions')
add_project_arguments(cpp_suppressions, language: ['cpp'])

##########################
# LV2 Path Configuration #
##########################

lv2dir = get_option('lv2dir')
prefix = get_option('prefix')
if lv2dir == ''
  if target_machine.system() == 'darwin' and prefix == '/'
    lv2dir = '/Library/Audio/Plug-Ins/LV2'
  elif target_machine.system() == 'haiku' and prefix == '/'
    lv2dir = '/boot/common/add-ons/lv2'
  elif target_machine.system() == 'windows' and prefix == 'C:/'
    lv2dir = 'C:/Program Files/Common/LV2'
  else
    lv2dir = prefix / get_option('libdir') / 'lv2'
  endif
endif

##########################
# Platform Configuration #
##########################

# TODO: Distinguish modules from libraries and move modules to a subdirectory
ingen_data_dir = (
  prefix / get_option('datadir') / 'ingen' # / versioned_name
)
ingen_module_dir = (
  prefix / get_option('libdir') # / versioned_name
)

# Use versioned name everywhere to support parallel major version installations
if host_machine.system() == 'windows'
  if get_option('default_library') == 'both'
    error('default_library=both is not supported on Windows')
  endif
  soversion = ''
else
  soversion = meson.project_version().split('.')[0]
endif

platform_defines = [
  '-DINGEN_DATA_DIR="@0@"'.format(ingen_data_dir),
  '-DINGEN_MODULE_DIR="@0@"'.format(ingen_module_dir),
  '-DINGEN_VERSION="@0@"'.format(meson.project_version()),
]

if host_machine.system() == 'darwin'
  platform_defines += [
    '-D_DARWIN_C_SOURCE',
    '-D_POSIX_C_SOURCE=200809L',
  ]
elif host_machine.system() == 'windows'
  platform_defines += [
    '-DINGEN_NO_POSIX',
  ]
elif host_machine.system() in ['gnu', 'linux']
  platform_defines += [
    '-D_POSIX_C_SOURCE=200809L',
    '-D_XOPEN_SOURCE=600',
  ]
endif

socket_code = '''#include <sys/socket.h>
int main(void) { return socket(AF_UNIX, SOCK_STREAM, 0); }'''

have_socket = cpp.compiles(socket_code, args: platform_defines, name: 'socket')

platform_defines += ['-DHAVE_SOCKET=@0@'.format(have_socket.to_int())]

#######################
# Common Dependencies #
#######################

boost_dep = dependency('boost', include_type: 'system')
thread_dep = dependency('threads')

serd_dep = dependency(
  'serd-0',
  fallback: ['serd', 'serd_dep'],
  version: '>= 0.30.4',
)

sord_dep = dependency(
  'sord-0',
  fallback: ['sord', 'sord_dep'],
  include_type: 'system',
  version: '>= 0.16.15',
)

sratom_dep = dependency(
  'sratom-0',
  fallback: ['sratom', 'sratom_dep'],
  version: '>= 0.6.0',
)

suil_dep = dependency(
  'suil-0',
  fallback: ['suil', 'suil_dep'],
  version: '>= 0.10.0',
)

lv2_dep = dependency(
  'lv2',
  fallback: ['lv2', 'lv2_dep'],
  include_type: 'system',
  version: '>= 1.18.0',
)

lilv_dep = dependency(
  'lilv-0',
  fallback: ['lilv', 'lilv_dep'],
  version: '>= 0.24.21',
)

raul_dep = dependency(
  'raul-2',
  fallback: ['raul', 'raul_dep'],
  include_type: 'system',
  version: '>= 2.0.0',
)

#######################
# Driver Dependencies #
#######################

portaudio_dep = dependency(
  'portaudio-2.0',
  include_type: 'system',
  required: get_option('portaudio'),
  version: '>= 2.0.0',
)

jack_dep = dependency(
  'jack',
  include_type: 'system',
  required: get_option('jack'),
  version: '>= 0.120.0',
)

jack_port_rename_code = '''#include <jack/jack.h>
int main(void) { return !!&jack_port_rename; }'''

platform_defines += '-DHAVE_JACK_PORT_RENAME=@0@'.format(
  cpp.compiles(
    jack_port_rename_code,
    args: platform_defines,
    dependencies: [jack_dep],
    name: 'jack_port_rename',
  ).to_int(),
)

#############
# Libraries #
#############

# Set appropriate arguments for building against the library type
if get_option('default_library') == 'static'
  add_project_arguments(['-DINGEN_STATIC'], language: ['cpp'])
endif

subdir('src')

########################
# Programs and Scripts #
########################

executable(
  'ingen',
  files('src/ingen/ingen.cpp'),
  cpp_args: cpp_suppressions + platform_defines,
  dependencies: [ingen_dep, raul_dep, serd_dep],
  implicit_include_directories: false,
  include_directories: ingen_include_dirs,
  install: true,
)

install_man(files('doc/ingen.1'))

subdir('scripts')

########
# Data #
########

install_data(
  files('src/ingen/ingen.desktop'),
  install_dir: get_option('datadir') / 'applications',
)

subdir('bundles')
subdir('icons')

#########
# Tests #
#########

subdir('tests')

if not meson.is_subproject()
  summary('Install prefix', get_option('prefix'))

  summary('Data', ingen_data_dir)
  summary('Executables', get_option('prefix') / get_option('bindir'))
  summary('LV2 bundles', lv2dir)
  summary('Man pages', get_option('prefix') / get_option('mandir'))
  summary('Modules', ingen_module_dir)
endif
