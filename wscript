#!/usr/bin/env python
import os
import Params
import autowaf

# Version of this package (even if built as a child)
INGEN_VERSION = '0.5.1'

# Variables for 'waf dist'
APPNAME = 'ingen'
VERSION = INGEN_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.add_option('--data-dir', type='string', dest='datadir',
			help="Ingen data install directory [Default: PREFIX/share/ingen]")
	opt.add_option('--module-dir', type='string', dest='moduledir',
			help="Ingen module install directory [Default: PREFIX/lib/ingen]")
	opt.add_option('--no-liblo', action='store_true', default=False, dest='no_liblo',
			help="Do not build OSC via liblo support, even if liblo exists")

def configure(conf):
	autowaf.configure(conf)
	autowaf.check_tool(conf, 'compiler_cxx')
	autowaf.check_pkg(conf, 'glibmm-2.4', destvar='GLIBMM', vnum='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'gthread-2.0', destvar='GTHREAD', vnum='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'gtkmm-2.4', destvar='GTKMM', vnum='2.11.12', mandatory=False)
	autowaf.check_pkg(conf, 'jack', destvar='JACK', vnum='0.109.0', mandatory=True)
	autowaf.check_pkg(conf, 'slv2', destvar='SLV2', vnum='0.6.0', mandatory=True)
	autowaf.check_pkg(conf, 'raul', destvar='RAUL', vnum='0.5.1', mandatory=True)
	autowaf.check_pkg(conf, 'flowcanvas', destvar='FLOWCANVAS', vnum='0.5.1', mandatory=False)
	autowaf.check_pkg(conf, 'libxml-2.0', destvar='XML2', vnum='2.6.0', mandatory=False)
	autowaf.check_pkg(conf, 'libglademm-2.4', destvar='GLADEMM', vnum='2.6.0', mandatory=False)
	autowaf.check_pkg(conf, 'libsoup-2.4', destvar='SOUP', vnum='2.4.0', mandatory=False)
	autowaf.check_header(conf, 'ladspa.h', 'HAVE_LADSPA', mandatory=False)
	if not Params.g_options.no_liblo:
		autowaf.check_pkg(conf, 'liblo', destvar='LIBLO', vnum='0.25', mandatory=False)
	autowaf.check_pkg(conf, 'redlandmm', destvar='REDLANDMM', vnum='0.0.0', mandatory=False)

	# Check for posix_memalign (OSX, amazingly, doesn't have it)
	fe = conf.create_function_enumerator()
	fe.headers = ['stdlib.h']
	fe.function = 'posix_memalign'
	fe.define = 'HAVE_POSIX_MEMALIGN'
	fe.run()

	build_gui = conf.env['HAVE_GLADEMM'] == 1 and conf.env['HAVE_FLOWCANVAS'] == 1

	conf.define('INGEN_VERSION', INGEN_VERSION)
	conf.define('BUILD_GUI', build_gui)
	conf.define('HAVE_JACK_MIDI', conf.env['HAVE_JACK'] == 1)
	conf.define('INGEN_DATA_DIR', os.path.normpath(conf.env['DATADIR'] + 'ingen'))
	conf.define('INGEN_MODULE_DIR', os.path.normpath(conf.env['LIBDIR'] + 'ingen'))
	conf.write_config_header('config.h')
	
	autowaf.print_summary(conf)
	autowaf.display_header('Ingen Configuration')
	autowaf.display_msg("Jack", str(conf.env['HAVE_JACK'] == 1), 'YELLOW')
	autowaf.display_msg("OSC", str(conf.env['HAVE_LIBLO'] == 1), 'YELLOW')
	autowaf.display_msg("HTTP", str(conf.env['HAVE_SOUP'] == 1), 'YELLOW')
	autowaf.display_msg("LV2", str(conf.env['HAVE_SLV2'] == 1), 'YELLOW')
	autowaf.display_msg("LADSPA", str(conf.env['HAVE_LADSPA'] == 1), 'YELLOW')
	autowaf.display_msg("Build GUI", str(conf.env['BUILD_GUI'] == 1), 'YELLOW')
	print

def build(bld):
	opts           = Params.g_options
	opts.datadir   = opts.datadir   or bld.env()['PREFIX'] + 'share'
	opts.moduledir = opts.moduledir or bld.env()['PREFIX'] + 'lib/ingen'
	
	# Modules
	bld.add_subdirs('src/engine')
	bld.add_subdirs('src/serialisation')
	bld.add_subdirs('src/module')
	bld.add_subdirs('src/shared')
	bld.add_subdirs('src/client')

	if bld.env()['BUILD_GUI']:
		bld.add_subdirs('src/gui')

	# Program
	bld.add_subdirs('src/ingen')
	
	# Documentation
	autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, srcdir, blddir)
	install_files('HTMLDIR', '', blddir + '/default/doc/html/*')

