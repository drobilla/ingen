#!/usr/bin/env python
import os
import subprocess
import waflib.Options as Options
import waflib.Utils as Utils
import waflib.extras.autowaf as autowaf

# Package version
INGEN_VERSION = '0.5.1'

# Mandatory waf variables
APPNAME = 'ingen'        # Package name for waf dist
VERSION = INGEN_VERSION  # Package version for waf dist
top     = '.'            # Source directory
out     = 'build'        # Build directory

def options(opt):
    opt.load('compiler_cxx')
    autowaf.set_options(opt)
    opt.add_option('--data-dir', type='string', dest='datadir',
                   help='Ingen data install directory [Default: PREFIX/share/ingen]')
    opt.add_option('--module-dir', type='string', dest='moduledir',
                   help='Ingen module install directory [Default: PREFIX/lib/ingen]')
    opt.add_option('--no-gui', action='store_true', dest='no_gui',
                   help='Do not build GUI')
    opt.add_option('--no-jack-session', action='store_true', default=False,
                   dest='no_jack_session',
                   help='Do not build JACK session support')
    opt.add_option('--no-socket', action='store_true', dest='no_socket',
                   help='Do not build Socket interface')
    opt.add_option('--test', action='store_true', dest='build_tests',
                   help='Build unit tests')

def configure(conf):
    conf.load('compiler_cxx')
    conf.load('python')

    autowaf.configure(conf)
    autowaf.display_header('Ingen Configuration')

    conf.check_cxx(cxxflags=["-std=c++0x"])
    conf.env.append_unique('CXXFLAGS', ['-std=c++0x'])

    autowaf.check_header(conf, 'cxx', 'boost/format.hpp')
    autowaf.check_header(conf, 'cxx', 'boost/intrusive/list.hpp')
    autowaf.check_header(conf, 'cxx', 'boost/optional.hpp')
    autowaf.check_header(conf, 'cxx', 'boost/utility.hpp')

    conf.check_python_version((2,4,0), mandatory=False)
    autowaf.check_pkg(conf, 'lv2', uselib_store='LV2',
                      atleast_version='1.6.2', mandatory=True)
    autowaf.check_pkg(conf, 'glibmm-2.4', uselib_store='GLIBMM',
                      atleast_version='2.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
                      atleast_version='2.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
                      atleast_version='0.120.0', mandatory=False)
    autowaf.check_pkg(conf, 'lilv-0', uselib_store='LILV',
                      atleast_version='0.16.1', mandatory=True)
    autowaf.check_pkg(conf, 'suil-0', uselib_store='SUIL',
                      atleast_version='0.2.0', mandatory=True)
    autowaf.check_pkg(conf, 'sratom-0', uselib_store='SRATOM',
                      atleast_version='0.2.1', mandatory=True)
    autowaf.check_pkg(conf, 'raul', uselib_store='RAUL',
                      atleast_version='0.8.5', mandatory=True)
    autowaf.check_pkg(conf, 'serd-0', uselib_store='SERD',
                      atleast_version='0.15.0', mandatory=False)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.9.0', mandatory=False)
    if not Options.options.no_gui:
        autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='GTKMM',
                          atleast_version='2.12.0', mandatory=False)
        autowaf.check_pkg(conf, 'gtkmm-2.4', uselib_store='NEW_GTKMM',
                          atleast_version='2.14.0', mandatory=False)
        autowaf.check_pkg(conf, 'webkit-1.0', uselib_store='WEBKIT',
                          atleast_version='1.4.0', mandatory=False)
        autowaf.check_pkg(conf, 'ganv-1', uselib_store='GANV',
                          atleast_version='1.2.1', mandatory=False)
    if not Options.options.no_socket:
        conf.check_cc(function_name='socket',
                      header_name='sys/socket.h',
                      define_name='HAVE_SOCKET',
                      mandatory=False)
    if not Options.options.no_jack_session:
        autowaf.define(conf, 'INGEN_JACK_SESSION', 1)

    conf.env.BUILD_TESTS = Options.options.build_tests
    if conf.env.BUILD_TESTS:
        conf.check_cxx(lib='gcov',
                       define_name='HAVE_GCOV',
                       mandatory=False)

        if conf.is_defined('HAVE_GCOV'):
            conf.env.INGEN_TEST_LIBS     = ['gcov']
            conf.env.INGEN_TEST_CXXFLAGS = ['-fprofile-arcs', '-ftest-coverage']
        else:
            conf.env.INGEN_TEST_LIBS     = []
            conf.env.INGEN_TEST_CXXFLAGS = []

    conf.check(function_name = 'posix_memalign',
               defines       = '_POSIX_SOURCE=1',
               header_name   = 'stdlib.h',
               define_name   = 'HAVE_POSIX_MEMALIGN',
               mandatory     = False)

    autowaf.define(conf, 'INGEN_VERSION', INGEN_VERSION)

    if not Options.options.no_gui and conf.is_defined('HAVE_GANV'):
        autowaf.define(conf, 'INGEN_BUILD_GUI', 1)

    if conf.is_defined('HAVE_JACK'):
        autowaf.define(conf, 'HAVE_JACK_MIDI', 1)

    autowaf.define(conf, 'INGEN_DATA_DIR',
                   os.path.join(conf.env.DATADIR, 'ingen'))
    autowaf.define(conf, 'INGEN_MODULE_DIR',
                   conf.env.LIBDIR)
    autowaf.define(conf, 'INGEN_BUNDLE_DIR',
                   os.path.join(conf.env.LV2DIR, 'ingen.lv2'))

    conf.write_config_header('ingen_config.h', remove=False)

    autowaf.display_msg(conf, "Jack", conf.is_defined('HAVE_JACK'))
    autowaf.display_msg(conf, "Jack session support",
                        conf.is_defined('INGEN_JACK_SESSION'))
    autowaf.display_msg(conf, "Socket interface", conf.is_defined('HAVE_SOCKET'))
    autowaf.display_msg(conf, "LV2", conf.is_defined('HAVE_LILV'))
    autowaf.display_msg(conf, "GUI", str(conf.env.INGEN_BUILD_GUI == 1))
    autowaf.display_msg(conf, "LV2 bundle", conf.env.INGEN_BUNDLE_DIR)
    autowaf.display_msg(conf, "HTML plugin documentation support",
                        conf.is_defined('HAVE_WEBKIT'))
    print('')

def build(bld):
    opts           = Options.options
    opts.datadir   = opts.datadir   or bld.env.PREFIX + 'share'
    opts.moduledir = opts.moduledir or bld.env.PREFIX + 'lib/ingen'

    # Headers
    for i in ['', 'client', 'serialisation']:
        bld.install_files('${INCLUDEDIR}/ingen/%s' % i,
                          bld.path.ant_glob('ingen/%s/*' % i))

    # Python modules
    if bld.env.PYTHONDIR:
        bld.install_files('${PYTHONDIR}/', 'scripts/ingen.py')

    # Modules
    bld.recurse('src')
    bld.recurse('src/serialisation')
    bld.recurse('src/server')
    bld.recurse('src/client')
    bld.recurse('src/socket')

    if bld.is_defined('INGEN_BUILD_GUI'):
        bld.recurse('src/gui')

    # Program
    obj = bld(features     = 'c cxx cxxprogram',
              source       = 'src/ingen/main.cpp',
              target       = bld.path.get_bld().make_node('ingen'),
              includes     = ['.'],
              use          = 'libingen',
              install_path = '${BINDIR}')
    autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV INGEN LV2')

    # Test program
    if bld.env.BUILD_TESTS:
        obj = bld(features     = 'cxx cxxprogram',
                  source       = 'tests/ingen_test.cpp',
                  target       = 'tests/ingen_test',
                  includes     = ['.'],
                  use          = 'libingen_profiled',
                  install_path = '',
                  lib          = bld.env.INGEN_TEST_LIBS,
                  cxxflags     = bld.env.INGEN_TEST_CXXFLAGS)
    autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV INGEN LV2 SRATOM')
        
    bld.install_files('${DATADIR}/applications', 'src/ingen/ingen.desktop')
    bld.install_files('${BINDIR}', 'scripts/ingenish', chmod=Utils.O755)
    bld.install_files('${BINDIR}', 'scripts/ingenams', chmod=Utils.O755)

    # Documentation
    autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, top, out)

    # Icons
    icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']
    for s in icon_sizes:
        bld.install_as(
                os.path.join(bld.env.DATADIR, 'icons', 'hicolor', s, 'apps', 'ingen.png'),
                'icons/' + s + '/ingen.png')

    bld.install_files('${LV2DIR}/ingen.lv2/',
                      bld.path.ant_glob('bundles/ingen.lv2/*'))

    for i in ['StereoInOut.ingen']:
        bld.install_files('${LV2DIR}/%s/' % str(i),
                          bld.path.ant_glob('bundles/%s/*' % str(i)))
        bld.symlink_as('${LV2DIR}/%s/libingen_lv2.so' % str(i),
                       bld.env.LV2DIR + '/ingen.lv2/libingen_lv2.so')

    bld.add_post_fun(autowaf.run_ldconfig)

def lint(ctx):
    subprocess.call('cpplint.py --filter=-whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/namespaces,-whitespace/line_length,-runtime/rtti,-runtime/references,-whitespace/blank_line,-runtime/sizeof,-readability/streams,-whitespace/operators,-whitespace/parens,-build/include,-build/storage_class `find -name *.cpp -or -name *.hpp`', shell=True)

def upload_docs(ctx):
    import shutil

    # Ontology documentation
    specgendir = '/usr/local/share/lv2specgen/'
    shutil.copy(specgendir + 'style.css', 'build')
    os.system('lv2specgen.py --list-email=ingen@drobilla.net --list-page=http://lists.drobilla.net/listinfo.cgi/ingen-drobilla.net bundles/ingen.lv2/ingen.ttl %s style.css build/ingen.html' % specgendir)
    os.system('rsync -avz -e ssh bundles/ingen.lv2/ingen.ttl drobilla@drobilla.net:~/drobilla.net/ns/')
    os.system('rsync -avz -e ssh build/ingen.html drobilla@drobilla.net:~/drobilla.net/ns/')
    os.system('rsync -avz -e ssh %s/style.css drobilla@drobilla.net:~/drobilla.net/ns/' % specgendir)

    # Doxygen documentation
    os.system('rsync -ravz --delete -e ssh build/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/ingen/')


def test(ctx):
    os.environ['PATH'] = 'tests' + os.pathsep + os.getenv('PATH')
    os.environ['LD_LIBRARY_PATH'] = os.path.join('src')
    os.environ['INGEN_MODULE_PATH'] = os.pathsep.join([
            os.path.join('src', 'server') ,
            os.path.join('src', 'serialisation')])

    autowaf.pre_test(ctx, APPNAME, dirs=['.', 'src', 'tests'])
    for i in ctx.path.ant_glob('tests/*.ttl'):
        autowaf.run_tests(ctx, APPNAME,
                          ['ingen_test ../tests/empty.ingen %s' % i.abspath()],
                          dirs=['.', 'src', 'tests'])
    autowaf.post_test(ctx, APPNAME, dirs=['.', 'src', 'tests'],
                      remove=['/usr*'])
