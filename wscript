#!/usr/bin/env python
import os
import Params

# Version of this package (even if built as a child)
INGEN_VERSION = '0.5.1'

# Variables for 'waf dist'
APPNAME = 'ingen'
VERSION = INGEN_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	opt.tool_options('compiler_cxx')
	opt.add_option('--data-dir', type='string', dest='datadir',
			help="Ingen data install directory [Default: PREFIX/share/ingen]")
	opt.add_option('--module-dir', type='string', dest='moduledir',
			help="Ingen module install directory [Default: PREFIX/lib/ingen]")

def configure(conf):
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
	if not conf.env['HAVE_LIBLO']:
		conf.check_pkg('liblo', destvar='LIBLO', vnum='0.25', mandatory=False)
	if not conf.env['HAVE_REDLANDMM']:
		conf.check_pkg('redlandmm', destvar='REDLANDMM', vnum='0.0.0', mandatory=False)
	conf.env['INGEN_VERSION'] = INGEN_VERSION
	conf.env.append_value('CCFLAGS', '-DCONFIG_H_PATH=\\\"waf-config.h\\\"')
	conf.env.append_value('CXXFLAGS', '-DCONFIG_H_PATH=\\\"waf-config.h\\\"')
	conf.write_config_header('waf-config.h')

def build(bld):
	opts = Params.g_options
	opts.datadir   = opts.datadir   or bld.env()['PREFIX'] + 'share'
	opts.moduledir = opts.moduledir or bld.env()['PREFIX'] + 'lib/ingen'
	bld.add_subdirs('src/engine')
	bld.add_subdirs('src/serialisation')
	bld.add_subdirs('src/module')
	bld.add_subdirs('src/shared')
	bld.add_subdirs('src/client')
	bld.add_subdirs('src/gui')
	bld.add_subdirs('src/ingen')

