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
	opt.tool_options('compiler_cxx')
	opt.add_option('--data-dir', type='string', dest='datadir',
			help="Ingen data install directory [Default: PREFIX/share/ingen]")
	opt.add_option('--module-dir', type='string', dest='moduledir',
			help="Ingen module install directory [Default: PREFIX/lib/ingen]")

def configure(conf):
	autowaf.configure(conf)
	if not conf.env['CXX']:
		conf.check_tool('compiler_cxx')
	if not conf.env['HAVE_GLIBMM']:
		conf.check_pkg('glibmm-2.4', destvar='GLIBMM', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_GTHREAD']:
		conf.check_pkg('gthread-2.0', destvar='GTHREAD', vnum='2.16.0', mandatory=True)
	if not conf.env['HAVE_GTKMM']:
		conf.check_pkg('gtkmm-2.4', destvar='GTKMM', vnum='2.11.12', mandatory=False)
	if not conf.env['HAVE_JACK']:
		conf.check_pkg('jack', destvar='JACK', vnum='0.107.0', mandatory=True)
	if not conf.env['HAVE_SLV2']:
		conf.check_pkg('slv2', destvar='SLV2', vnum='0.6.0', mandatory=True)
	if not conf.env['HAVE_RAUL']:
		conf.check_pkg('raul', destvar='RAUL', vnum='0.5.1', mandatory=True)
	if not conf.env['HAVE_FLOWCANVAS']:
		conf.check_pkg('flowcanvas', destvar='FLOWCANVAS', vnum='0.5.1', mandatory=True)
	if not conf.env['HAVE_XML2']:
		conf.check_pkg('libxml-2.0', destvar='XML2', vnum='2.6.0', mandatory=False)
	if not conf.env['HAVE_GLADEMM']:
		conf.check_pkg('libglademm-2.4', destvar='GLADEMM', vnum='2.6.0', mandatory=False)
	if not conf.env['HAVE_SOUP']:
		conf.check_pkg('libsoup-2.4', destvar='SOUP', vnum='2.4.0', mandatory=False)
	if not conf.env['HAVE_LADSPA']:
		conf.check_header('ladspa.h', 'HAVE_LADSPA', mandatory=False)
	if not conf.env['HAVE_LIBLO']:
		conf.check_pkg('liblo', destvar='LIBLO', vnum='0.25', mandatory=False)
	if not conf.env['HAVE_REDLANDMM']:
		conf.check_pkg('redlandmm', destvar='REDLANDMM', vnum='0.0.0', mandatory=False)
	conf.env['INGEN_VERSION'] = INGEN_VERSION
	conf.env['BUILD_GUI'] = bool(conf.env['GLADEMM'])
	conf.write_config_header('waf-config.h')
	
	autowaf.print_summary(conf)
	autowaf.display_header('Ingen Configuration')
	autowaf.display_msg("Jack", str(bool(conf.env['HAVE_JACK_DBUS'])), 'YELLOW')
	autowaf.display_msg("OSC", str(bool(conf.env['HAVE_LIBLO'])), 'YELLOW')
	autowaf.display_msg("HTTP", str(bool(conf.env['HAVE_SOUP'])), 'YELLOW')
	autowaf.display_msg("LV2", str(bool(conf.env['HAVE_SLV2'])), 'YELLOW')
	autowaf.display_msg("LADSPA", str(bool(conf.env['HAVE_LADSPA'])), 'YELLOW')
	print

def build(bld):
	opts = Params.g_options
	opts.datadir   = opts.datadir   or bld.env()['PREFIX'] + 'share'
	opts.moduledir = opts.moduledir or bld.env()['PREFIX'] + 'lib/ingen'
	
	# Modules
	bld.add_subdirs('src/engine')
	bld.add_subdirs('src/serialisation')
	bld.add_subdirs('src/module')
	bld.add_subdirs('src/shared')
	bld.add_subdirs('src/client')
	bld.add_subdirs('src/gui')

	# Program
	bld.add_subdirs('src/ingen')
	
	# Documentation
	autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, srcdir, blddir)
	install_files('PREFIX', 'share/doc/ingen', blddir + '/default/doc/html/*')

