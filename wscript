#!/usr/bin/env python
import os
import autowaf
import Options

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
	opt.add_option('--no-ladspa', action='store_true', default=False, dest='no_ladspa',
		       help="Do not build LADSPA support, even if ladspa.h is found")
	opt.add_option('--no-osc', action='store_true', default=False, dest='no_osc',
		       help="Do not build OSC via liblo support, even if liblo exists")
	opt.add_option('--no-http', action='store_true', default=False, dest='no_http',
		       help="Do not build HTTP via libsoup support, even if libsoup exists")
	opt.add_option('--log-debug', action='store_true', default=False, dest='log_debug',
		       help="Print debugging output")
	opt.add_option('--liblo-bundles', action='store_true', default=False, dest='liblo_bundles',
		       help="Use liblo bundle support (experimental, requires patched liblo)")

def configure(conf):
	conf.line_just = max(conf.line_just, 67)
	autowaf.configure(conf)
	autowaf.display_header('Ingen Configuration')
	conf.check_tool('compiler_cxx')
	autowaf.check_pkg(conf, 'glibmm-2.4', uselib_store='GLIBMM',
			  atleast_version='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
			  atleast_version='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='GTKMM',
			  atleast_version='2.11.12', mandatory=False)
	autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='NEW_GTKMM',
			  atleast_version='2.14.0', mandatory=False)
	autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
			  atleast_version='0.109.0', mandatory=True)
	autowaf.check_pkg(conf, 'slv2', uselib_store='SLV2',
			  atleast_version='0.6.0', mandatory=True)
	autowaf.check_pkg(conf, 'raul', uselib_store='RAUL',
			  atleast_version='0.6.2', mandatory=True)
	autowaf.check_pkg(conf, 'flowcanvas', uselib_store='FLOWCANVAS',
			  atleast_version='0.7.0', mandatory=False)
	autowaf.check_pkg(conf, 'libxml-2.0', uselib_store='XML2',
			  atleast_version='2.6.0', mandatory=False)
	autowaf.check_pkg(conf, 'libglademm-2.4', uselib_store='GLADEMM',
			  atleast_version='2.6.0', mandatory=False)
	autowaf.check_pkg(conf, 'redlandmm', uselib_store='REDLANDMM',
			  atleast_version='0.0.0', mandatory=False)
	if not Options.options.no_http:
		autowaf.check_pkg(conf, 'libsoup-2.4', uselib_store='SOUP',
				  atleast_version='2.4.0', mandatory=False)
	if not Options.options.no_ladspa:
		autowaf.check_header(conf, 'ladspa.h', 'HAVE_LADSPA_H', mandatory=False)
	if not Options.options.no_osc:
		autowaf.check_pkg(conf, 'liblo', uselib_store='LIBLO',
				  atleast_version='0.25', mandatory=False)

	# Check for posix_memalign (OSX, amazingly, doesn't have it)
	conf.check(function_name='posix_memalign',
		   header_name='stdlib.h',
		   define_name='HAVE_POSIX_MEMALIGN')

	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/lv2core/lv2.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/atom/atom.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/contexts/contexts.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event-helpers.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/resize-port/resize-port.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-map/uri-map.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-unmap/uri-unmap.h')

	build_gui = conf.env['HAVE_GLADEMM'] == 1 and conf.env['HAVE_FLOWCANVAS'] == 1

	conf.define('INGEN_VERSION', INGEN_VERSION)
	conf.define('BUILD_INGEN_GUI', int(build_gui))
	conf.define('HAVE_JACK_MIDI', int(conf.env['HAVE_JACK'] == 1))
	if conf.env['BUNDLE']:
		conf.define('INGEN_DATA_DIR', os.path.join(
				conf.env['DATADIRNAME'], 'ingen'))
		conf.define('INGEN_MODULE_DIR', conf.env['LIBDIRNAME'])
	else:
		conf.define('INGEN_DATA_DIR', os.path.join(
				conf.env['DATADIR'], 'ingen'))
		conf.define('INGEN_MODULE_DIR',	conf.env['LIBDIR'])
	
	if Options.options.log_debug:
		conf.define('LOG_DEBUG', 1)
	
	if Options.options.liblo_bundles:
		conf.define('LIBLO_BUNDLES', 1)

	conf.write_config_header('ingen-config.h')

	autowaf.display_msg(conf, "Jack", str(conf.env['HAVE_JACK'] == 1))
	autowaf.display_msg(conf, "OSC", str(conf.env['HAVE_LIBLO'] == 1))
	autowaf.display_msg(conf, "HTTP", str(conf.env['HAVE_SOUP'] == 1))
	autowaf.display_msg(conf, "LV2", str(conf.env['HAVE_SLV2'] == 1))
	autowaf.display_msg(conf, "LADSPA", str(conf.env['HAVE_LADSPA_H'] == 1))
	autowaf.display_msg(conf, "GUI", str(conf.env['BUILD_INGEN_GUI'] == 1))
	print

def build(bld):
	opts           = Options.options
	opts.datadir   = opts.datadir   or bld.env['PREFIX'] + 'share'
	opts.moduledir = opts.moduledir or bld.env['PREFIX'] + 'lib/ingen'
	
	# Headers
	bld.install_files('${INCLUDEDIR}/ingen/interface', 'src/common/interface/*.hpp')

	# Modules
	bld.add_subdirs('src/engine')
	bld.add_subdirs('src/serialisation')
	bld.add_subdirs('src/module')
	bld.add_subdirs('src/shared')
	bld.add_subdirs('src/client')

	if bld.env['BUILD_INGEN_GUI']:
		bld.add_subdirs('src/gui')

	# Program
	bld.add_subdirs('src/ingen')

	# Documentation
	autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, srcdir, blddir)
	bld.install_files('${HTMLDIR}', blddir + '/default/doc/html/*')

	# Icons
	icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
	for s in icon_sizes:
		bld.install_as(
			os.path.join(bld.env['DATADIR'], 'icons', 'hicolor', s, 'apps', 'ingen.png'),
			'icons/' + s + '/ingen.png')

