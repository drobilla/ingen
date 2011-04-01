#!/usr/bin/env python
import os

import waflib.Options as Options
from waflib.extras import autowaf as autowaf

# Version of this package (even if built as a child)
INGEN_VERSION = '0.5.1'

# Variables for 'waf dist'
APPNAME = 'ingen'
VERSION = INGEN_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
	autowaf.set_options(opt)
	opt.add_option('--data-dir', type='string', dest='datadir',
		       help="Ingen data install directory [Default: PREFIX/share/ingen]")
	opt.add_option('--module-dir', type='string', dest='moduledir',
		       help="Ingen module install directory [Default: PREFIX/lib/ingen]")
	opt.add_option('--no-gui', action='store_true', default=False, dest='no_gui',
		       help="Do not build GUI")
	opt.add_option('--no-jack-session', action='store_true', default=False,
			dest='no_jack_session',
			help="Do not build JACK session support")
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
	conf.load('compiler_cxx')
	autowaf.check_pkg(conf, 'glibmm-2.4', uselib_store='GLIBMM',
			  atleast_version='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
			  atleast_version='2.14.0', mandatory=True)
	autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
			  atleast_version='0.109.0', mandatory=True)
	autowaf.check_pkg(conf, 'jack', uselib_store='NEW_JACK',
			  atleast_version='0.120.0', mandatory=False)
	autowaf.check_pkg(conf, 'slv2', uselib_store='SLV2',
			  atleast_version='0.7.0', mandatory=True)
	autowaf.check_pkg(conf, 'raul', uselib_store='RAUL',
			  atleast_version='0.8.0', mandatory=True)
	autowaf.check_pkg(conf, 'flowcanvas', uselib_store='FLOWCANVAS',
			  atleast_version='0.8.0', mandatory=False)
	autowaf.check_pkg(conf, 'sord', uselib_store='SORD',
			  atleast_version='0.1.0', mandatory=False)
	if not Options.options.no_gui:
		autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='GTKMM',
		                  atleast_version='2.11.12', mandatory=False)
		autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='NEW_GTKMM',
		                  atleast_version='2.14.0', mandatory=False)
		autowaf.check_pkg(conf, 'libglademm-2.4', uselib_store='GLADEMM',
		                  atleast_version='2.6.0', mandatory=False)
	if not Options.options.no_http:
		autowaf.check_pkg(conf, 'libsoup-2.4', uselib_store='SOUP',
				  atleast_version='2.4.0', mandatory=False)
	if not Options.options.no_osc:
		autowaf.check_pkg(conf, 'liblo', uselib_store='LIBLO',
				  atleast_version='0.25', mandatory=False)
	if not Options.options.no_jack_session:
		if conf.is_defined('HAVE_NEW_JACK'):
			autowaf.define(conf, 'INGEN_JACK_SESSION', 1)

	# Check for posix_memalign (OSX, amazingly, doesn't have it)
	conf.check(function_name='posix_memalign',
	           header_name='stdlib.h',
	           define_name='HAVE_POSIX_MEMALIGN',
	           mandatory=False)

	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/lv2core/lv2.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/atom/atom.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/contexts/contexts.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event-helpers.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/event/event.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/resize-port/resize-port.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-map/uri-map.h')
	autowaf.check_header(conf, 'lv2/lv2plug.in/ns/ext/uri-unmap/uri-unmap.h')

	autowaf.define(conf, 'INGEN_VERSION', INGEN_VERSION)

	if (not Options.options.no_gui
		and conf.is_defined('HAVE_GLADEMM')
		and conf.is_defined('HAVE_FLOWCANVAS')):
		autowaf.define(conf, 'INGEN_BUILD_GUI', 1)
	
	if conf.is_defined('HAVE_JACK'):
		autowaf.define(conf, 'HAVE_JACK_MIDI', 1)

	autowaf.define(conf, 'INGEN_DATA_DIR', os.path.join(conf.env['DATADIR'], 'ingen'))
	autowaf.define(conf, 'INGEN_MODULE_DIR', conf.env['LIBDIR'])

	if Options.options.log_debug:
		autowaf.define(conf, 'RAUL_LOG_DEBUG', 1)

	if Options.options.liblo_bundles:
		autowaf.define(conf, 'LIBLO_BUNDLES', 1)

	conf.write_config_header('ingen-config.h', remove=False)

	autowaf.display_msg(conf, "Jack", conf.is_defined('HAVE_JACK'))
	autowaf.display_msg(conf, "Jack session support",
	                    conf.is_defined('INGEN_JACK_SESSION'))
	autowaf.display_msg(conf, "OSC", conf.is_defined('HAVE_LIBLO'))
	autowaf.display_msg(conf, "HTTP", conf.is_defined('HAVE_SOUP'))
	autowaf.display_msg(conf, "LV2", conf.is_defined('HAVE_SLV2'))
	autowaf.display_msg(conf, "GUI", str(conf.env['INGEN_BUILD_GUI'] == 1))
	print('')

def build(bld):
	opts           = Options.options
	opts.datadir   = opts.datadir   or bld.env['PREFIX'] + 'share'
	opts.moduledir = opts.moduledir or bld.env['PREFIX'] + 'lib/ingen'

	# Headers
	bld.install_files('${INCLUDEDIR}/ingen/interface',
			  bld.path.ant_glob('src/common/interface/*.hpp'))

	# Modules
	bld.recurse('src/engine')
	bld.recurse('src/serialisation')
	bld.recurse('src/module')
	bld.recurse('src/shared')
	bld.recurse('src/client')

	if bld.is_defined('INGEN_BUILD_GUI'):
		bld.recurse('src/gui')

	# Program
	obj = bld(features = 'c cxx cxxprogram')
	obj.target       = 'ingen'
	obj.source       = 'src/ingen/main.cpp'
	obj.includes     = ['.', 'src', 'src/common']
	obj.defines      = 'VERSION="' + bld.env['INGEN_VERSION'] + '"'
	obj.use          = 'libingen_module libingen_shared'
	obj.install_path = '${BINDIR}'
	autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LV2CORE SLV2 INGEN LIBLO SOUP')

	bld.install_files('${DATADIR}/applications', 'src/ingen/ingen.desktop')

	# Documentation
	autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, top, out)

	# Icons
	icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
	for s in icon_sizes:
		bld.install_as(
			os.path.join(bld.env['DATADIR'], 'icons', 'hicolor', s, 'apps', 'ingen.png'),
			'icons/' + s + '/ingen.png')

	bld.add_post_fun(autowaf.run_ldconfig)
