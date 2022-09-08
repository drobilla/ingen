# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR GPL-3.0-or-later

################
# Dependencies #
################

gui_defines = platform_defines

glibmm_dep = dependency('glibmm-2.4',
                        version: '>= 2.14.0',
                        include_type: 'system',
                        required: get_option('gui'))

gthread_dep = dependency('gthread-2.0',
                         version: '>= 2.14.0',
                         include_type: 'system',
                         required: get_option('gui'))

gtkmm_dep = dependency('gtkmm-2.4',
                       version: '>= 2.14.0',
                       include_type: 'system',
                       required: get_option('gui'))

ganv_dep = dependency('ganv-1',
                      version: '>= 1.5.2',
                      fallback: ['ganv', 'ganv_dep'],
                      required: get_option('gui'))

webkit_dep = dependency('webkit-1.0',
                        version: '>= 1.4.0',
                        include_type: 'system',
                        required: false)

build_gui = (glibmm_dep.found() and
             gthread_dep.found() and
             gtkmm_dep.found() and
             ganv_dep.found())

if webkit_dep.found()
  gui_defines += ['-DHAVE_WEBKIT=1']
else
  gui_defines += ['-DHAVE_WEBKIT=0']
endif

##########
# Module #
##########

if build_gui
  gui_sources = files(
    'App.cpp',
    'Arc.cpp',
    'BreadCrumbs.cpp',
    'ConnectWindow.cpp',
    'GraphBox.cpp',
    'GraphCanvas.cpp',
    'GraphPortModule.cpp',
    'GraphTreeWindow.cpp',
    'GraphView.cpp',
    'GraphWindow.cpp',
    'LoadGraphWindow.cpp',
    'LoadPluginWindow.cpp',
    'MessagesWindow.cpp',
    'NewSubgraphWindow.cpp',
    'NodeMenu.cpp',
    'NodeModule.cpp',
    'ObjectMenu.cpp',
    'PluginMenu.cpp',
    'Port.cpp',
    'PortMenu.cpp',
    'PropertiesWindow.cpp',
    'RDFS.cpp',
    'RenameWindow.cpp',
    'Style.cpp',
    'SubgraphModule.cpp',
    'ThreadedLoader.cpp',
    'URIEntry.cpp',
    'WidgetFactory.cpp',
    'WindowFactory.cpp',
    'ingen_gui.cpp',
  )

  gui_dependencies = [
    boost_dep,
    ganv_dep,
    glibmm_dep,
    gthread_dep,
    gtkmm_dep,
    ingen_client_dep,
    ingen_dep,
    lilv_dep,
    raul_dep,
    sigcpp_dep,
    suil_dep,
    thread_dep,
    webkit_dep,
  ]

  gui_suppressions = []
  if cpp.get_id() == 'clang'
    gui_suppressions += [
      '-Wno-reserved-identifier', # Ganv
    ]
  endif

  gui_suppressions = cpp.get_supported_arguments(gui_suppressions)
  gui_suppressions += cpp_suppressions

  gui_args = gui_suppressions + gui_defines + ['-DINGEN_GUI_INTERNAL']

  libingen_gui = shared_library(
    'ingen_gui',
    gui_sources,
    cpp_args: gui_args,
    dependencies: gui_dependencies,
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: ingen_include_dirs,
    install: true,
    install_dir: ingen_module_dir,
  )

  ingen_gui_dep = declare_dependency(
    dependencies: gui_dependencies,
    link_with: libingen_gui,
  )

  ##########
  # LV2 UI #
  ##########

  ingen_gui_lv2 = shared_library(
    'ingen_gui_lv2',
    files('ingen_gui_lv2.cpp'),
    cpp_args: gui_args,
    dependencies: [ingen_gui_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    include_directories: ingen_include_dirs,
    install: true,
    install_dir: lv2dir / 'ingen.lv2',
  )

  ###############
  # Shared Data #
  ###############

  config = configuration_data()
  config.set('INGEN_VERSION', meson.project_version())

  configure_file(configuration: config,
                 input: files('ingen_gui.ui.in'),
                 output: 'ingen_gui.ui',
                 install: true,
                 install_dir: ingen_data_dir)

  configure_file(copy: true,
                 input: files('ingen_style.rc'),
                 output: '@PLAINNAME@',
                 install: true,
                 install_dir: ingen_data_dir)
endif
