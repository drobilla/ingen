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
    opt.load('compiler_cxx')
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
    conf.load('compiler_cxx')
    autowaf.configure(conf)
    conf.line_just = 48

    Options.options.no_http = True
    Options.options.no_osc  = True

    autowaf.display_header('Ingen Configuration')
    autowaf.check_pkg(conf, 'glibmm-2.4', uselib_store='GLIBMM',
                      atleast_version='2.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
                      atleast_version='2.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
                      atleast_version='0.109.0', mandatory=True)
    autowaf.check_pkg(conf, 'jack', uselib_store='NEW_JACK',
                      atleast_version='0.120.0', mandatory=False)
    autowaf.check_pkg(conf, 'lilv-0', uselib_store='LILV',
                      atleast_version='0.0.0', mandatory=True)
    autowaf.check_pkg(conf, 'suil-0', uselib_store='SUIL',
                      atleast_version='0.2.0', mandatory=True)
    autowaf.check_pkg(conf, 'sratom-0', uselib_store='SRATOM',
                      atleast_version='0.1.0', mandatory=True)
    autowaf.check_pkg(conf, 'raul', uselib_store='RAUL',
                      atleast_version='0.8.3', mandatory=True)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.7.0', mandatory=False)
    if not Options.options.no_gui:
        autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='GTKMM',
                          atleast_version='2.12.0', mandatory=False)
        autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='NEW_GTKMM',
                          atleast_version='2.14.0', mandatory=False)
        autowaf.check_pkg(conf, 'webkit-1.0', uselib_store='WEBKIT',
                          atleast_version='1.4.0', mandatory=False)
        autowaf.check_pkg(conf, 'ganv-1', uselib_store='GANV',
                          atleast_version='1.0.0', mandatory=False)
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
               defines='_POSIX_SOURCE=1',
               header_name='stdlib.h',
               define_name='HAVE_POSIX_MEMALIGN',
               mandatory=False)

    autowaf.check_pkg(conf, 'lv2', atleast_version='0.3.0', uselib_store='LV2')

    autowaf.define(conf, 'INGEN_VERSION', INGEN_VERSION)

    if not Options.options.no_gui and conf.is_defined('HAVE_GANV'):
        autowaf.define(conf, 'INGEN_BUILD_GUI', 1)

    if conf.is_defined('HAVE_JACK'):
        autowaf.define(conf, 'HAVE_JACK_MIDI', 1)

    autowaf.define(conf, 'INGEN_DATA_DIR',
                   os.path.join(conf.env['DATADIR'], 'ingen'))
    autowaf.define(conf, 'INGEN_MODULE_DIR',
                   conf.env['LIBDIR'])
    autowaf.define(conf, 'INGEN_BUNDLE_DIR',
                   os.path.join(conf.env['LV2DIR'], 'ingen.lv2'))

    if Options.options.log_debug:
        autowaf.define(conf, 'RAUL_LOG_DEBUG', 1)

    if Options.options.liblo_bundles:
        autowaf.define(conf, 'LIBLO_BUNDLES', 1)

    conf.write_config_header('ingen_config.h', remove=False)

    autowaf.display_msg(conf, "Jack", conf.is_defined('HAVE_JACK'))
    autowaf.display_msg(conf, "Jack session support",
                        conf.is_defined('INGEN_JACK_SESSION'))
    autowaf.display_msg(conf, "OSC", conf.is_defined('HAVE_LIBLO'))
    autowaf.display_msg(conf, "HTTP", conf.is_defined('HAVE_SOUP'))
    autowaf.display_msg(conf, "LV2", conf.is_defined('HAVE_LILV'))
    autowaf.display_msg(conf, "GUI", str(conf.env['INGEN_BUILD_GUI'] == 1))
    autowaf.display_msg(conf, "LV2 Bundle", conf.env['INGEN_BUNDLE_DIR'])
    autowaf.display_msg(conf, "HTML plugin documentation support",
                        conf.is_defined('HAVE_WEBKIT'))
    print('')

def build(bld):
    opts           = Options.options
    opts.datadir   = opts.datadir   or bld.env['PREFIX'] + 'share'
    opts.moduledir = opts.moduledir or bld.env['PREFIX'] + 'lib/ingen'

    # Headers
    for i in ['', 'client', 'serialisation', 'shared']:
        bld.install_files('${INCLUDEDIR}/ingen/%s' % i,
                          bld.path.ant_glob('ingen/%s/*' % i))

    # Modules
    bld.recurse('src/shared')
    bld.recurse('src/serialisation')
    bld.recurse('src/server')
    bld.recurse('src/client')
    bld.recurse('src/http')
    bld.recurse('src/osc')

    if bld.is_defined('INGEN_BUILD_GUI'):
        bld.recurse('src/gui')

    # Program
    obj = bld(features     = 'c cxx cxxprogram',
              source       = 'src/ingen/main.cpp',
              target       = bld.path.get_bld().make_node('ingen'),
              includes     = ['.', '../..'],
              defines      = 'VERSION="' + bld.env['INGEN_VERSION'] + '"',
              use          = 'libingen_shared',
              install_path = '${BINDIR}')
    autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV INGEN LV2')

    bld.install_files('${DATADIR}/applications', 'src/ingen/ingen.desktop')

    # Documentation
    autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, top, out)

    # Icons
    icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
    for s in icon_sizes:
        bld.install_as(
                os.path.join(bld.env['DATADIR'], 'icons', 'hicolor', s, 'apps', 'ingen.png'),
                'icons/' + s + '/ingen.png')

    # Template patches/plugins bundle
    bld.install_files('${LV2DIR}/ingen.lv2/',
                      bld.path.ant_glob('ingen.lv2/*'))

    bld.add_post_fun(autowaf.run_ldconfig)
