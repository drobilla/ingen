#!/usr/bin/env python
import os
import subprocess
import waflib.Logs as Logs
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
    opt.load('python')
    opt.load('lv2')
    opt.recurse('src/gui')
    autowaf.set_options(opt, test=True)
    opt.add_option('--data-dir', type='string', dest='datadir',
                   help='Ingen data install directory [Default: PREFIX/share/ingen]')
    opt.add_option('--module-dir', type='string', dest='moduledir',
                   help='Ingen module install directory [Default: PREFIX/lib/ingen]')
    opt.add_option('--no-gui', action='store_true', dest='no_gui',
                   help='Do not build GUI')
    opt.add_option('--no-client', action='store_true', dest='no_client',
                   help='Do not build client library (or GUI)')
    opt.add_option('--no-jack', action='store_true', dest='no_jack',
                   help='Do not build jack backend (for ingen.lv2 only)')
    opt.add_option('--no-plugin', action='store_true', dest='no_plugin',
                   help='Do not build ingen.lv2 plugin')
    opt.add_option('--no-python', action='store_true', dest='no_python',
                   help='Do not install Python bindings')
    opt.add_option('--no-webkit', action='store_true', dest='no_webkit',
                   help='Do not use webkit to display plugin documentation')
    opt.add_option('--no-jack-session', action='store_true', default=False,
                   dest='no_jack_session',
                   help='Do not build JACK session support')
    opt.add_option('--no-socket', action='store_true', dest='no_socket',
                   help='Do not build Socket interface')
    opt.add_option('--debug-urids', action='store_true', dest='debug_urids',
                   help='Print a trace of URI mapping')
    opt.add_option('--portaudio', action='store_true', default=False,
                   dest='portaudio',
                   help='Build PortAudio backend')

def configure(conf):
    autowaf.display_header('Ingen Configuration')
    conf.line_just = 44
    conf.load('compiler_cxx')
    conf.load('lv2')
    if not Options.options.no_python:
        conf.load('python')

    autowaf.configure(conf)
    autowaf.set_cxx11_mode(conf)

    conf.check_cxx(header_name='boost/format.hpp')
    conf.check_cxx(header_name='boost/shared_ptr.hpp')
    conf.check_cxx(header_name='boost/utility.hpp')
    conf.check_cxx(header_name='boost/weak_ptr.hpp')
    conf.check_cxx(msg='Checking for thread_local keyword',
                   mandatory=False,
                   fragment='thread_local int i = 0; int main() {}',
                   define_name='INGEN_HAVE_THREAD_LOCAL')
    if not conf.is_defined('INGEN_HAVE_THREAD_LOCAL'):
        conf.check_cxx(msg='Checking for __thread keyword',
                       mandatory=False,
                       fragment='__thread int i = 0; int main() {}',
                       define_name='INGEN_HAVE_THREAD_BUILTIN')

    autowaf.check_pkg(conf, 'lv2', uselib_store='LV2',
                      atleast_version='1.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'glibmm-2.4', uselib_store='GLIBMM',
                      atleast_version='2.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'gthread-2.0', uselib_store='GTHREAD',
                      atleast_version='2.14.0', mandatory=True)
    autowaf.check_pkg(conf, 'lilv-0', uselib_store='LILV',
                      atleast_version='0.21.5', mandatory=True)
    autowaf.check_pkg(conf, 'suil-0', uselib_store='SUIL',
                      atleast_version='0.2.0', mandatory=True)
    autowaf.check_pkg(conf, 'sratom-0', uselib_store='SRATOM',
                      atleast_version='0.4.6', mandatory=True)
    autowaf.check_pkg(conf, 'raul', uselib_store='RAUL',
                      atleast_version='0.8.9', mandatory=True)
    autowaf.check_pkg(conf, 'serd-0', uselib_store='SERD',
                      atleast_version='0.18.0', mandatory=False)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.12.0', mandatory=False)
    autowaf.check_pkg(conf, 'portaudio-2.0', uselib_store='PORTAUDIO',
                      atleast_version='2.0.0', mandatory=False)

    conf.check(function_name = 'posix_memalign',
               defines       = '_POSIX_C_SOURCE=200809L',
               header_name   = 'stdlib.h',
               define_name   = 'HAVE_POSIX_MEMALIGN',
               mandatory     = False)

    conf.check(function_name = 'isatty',
               header_name   = 'unistd.h',
               defines       = '_POSIX_C_SOURCE=200809L',
               define_name   = 'HAVE_ISATTY',
               mandatory     = False)

    conf.check(function_name = 'vasprintf',
               header_name   = 'stdio.h',
               defines       = '_GNU_SOURCE=1',
               define_name   = 'HAVE_VASPRINTF',
               mandatory     = False)

    conf.check(define_name   = 'HAVE_LIBDL',
               lib           = 'dl',
               mandatory     = False)

    if not Options.options.no_socket:
        conf.check(function_name = 'socket',
                   header_name   = 'sys/socket.h',
                   define_name   = 'HAVE_SOCKET',
                   mandatory     = False)

    if not Options.options.no_python:
        conf.check_python_version((2,4,0), mandatory=False)

    if not Options.options.no_plugin:
        autowaf.define(conf, 'INGEN_BUILD_LV2', 1)

    if not Options.options.no_jack:
        autowaf.check_pkg(conf, 'jack', uselib_store='JACK',
                          atleast_version='0.120.0', mandatory=False)
        conf.check(function_name = 'jack_set_property',
                   header_name   = 'jack/metadata.h',
                   define_name   = 'HAVE_JACK_METADATA',
                   uselib        = 'JACK',
                   mandatory     = False)
        conf.check(function_name = 'jack_port_rename',
                   header_name   = 'jack/jack.h',
                   define_name   = 'HAVE_JACK_PORT_RENAME',
                   uselib        = 'JACK',
                   mandatory     = False)
        if not Options.options.no_jack_session:
            autowaf.define(conf, 'INGEN_JACK_SESSION', 1)

    if Options.options.debug_urids:
        autowaf.define(conf, 'INGEN_DEBUG_URIDS', 1)

    if conf.env.BUILD_TESTS:
        conf.env.INGEN_TEST_LINKFLAGS = []
        conf.env.INGEN_TEST_CXXFLAGS  = []
        if not conf.env.NO_COVERAGE:
            conf.env.INGEN_TEST_CXXFLAGS  += ['--coverage']
            conf.env.INGEN_TEST_LINKFLAGS += ['--coverage']

    if conf.check(cflags=['-pthread'], mandatory=False):
        conf.env.PTHREAD_CFLAGS    = ['-pthread']
        if conf.env.CXX_NAME != 'clang':
            conf.env.PTHREAD_LINKFLAGS = ['-pthread']
    elif conf.check(linkflags=['-lpthread'], mandatory=False):
        conf.env.PTHREAD_CFLAGS    = []
        conf.env.PTHREAD_LINKFLAGS = ['-lpthread']
    else:
        conf.env.PTHREAD_CFLAGS    = []
        conf.env.PTHREAD_LINKFLAGS = []

    autowaf.define(conf, 'INGEN_SHARED', 1);
    autowaf.define(conf, 'INGEN_VERSION', INGEN_VERSION)

    if not Options.options.no_client:
        autowaf.define(conf, 'INGEN_BUILD_CLIENT', 1)
    else:
        Options.options.no_gui = True

    if not Options.options.no_gui:
        conf.recurse('src/gui')

    if conf.env.HAVE_JACK:
        autowaf.define(conf, 'HAVE_JACK_MIDI', 1)

    autowaf.define(conf, 'INGEN_DATA_DIR',
                   os.path.join(conf.env.DATADIR, 'ingen'))
    autowaf.define(conf, 'INGEN_MODULE_DIR',
                   conf.env.LIBDIR)
    autowaf.define(conf, 'INGEN_BUNDLE_DIR',
                   os.path.join(conf.env.LV2DIR, 'ingen.lv2'))

    conf.write_config_header('ingen_config.h', remove=False)

    autowaf.display_msg(conf, "GUI", bool(conf.env.INGEN_BUILD_GUI))
    autowaf.display_msg(conf, "HTML plugin documentation support",
                        bool(conf.env.HAVE_WEBKIT))
    autowaf.display_msg(conf, "Jack driver", bool(conf.env.HAVE_JACK))
    autowaf.display_msg(conf, "Jack session support",
                        bool(conf.env.INGEN_JACK_SESSION))
    autowaf.display_msg(conf, "Jack metadata support",
                        conf.is_defined('HAVE_JACK_METADATA'))
    autowaf.display_msg(conf, "LV2 plugin driver", bool(conf.env.INGNE_BUILD_LV2))
    autowaf.display_msg(conf, "LV2 bundle", conf.env.INGEN_BUNDLE_DIR)
    autowaf.display_msg(conf, "LV2 plugin support", bool(conf.env.HAVE_LILV))
    autowaf.display_msg(conf, "Socket interface", conf.is_defined('HAVE_SOCKET'))
    print('')

def build(bld):
    opts           = Options.options
    opts.datadir   = opts.datadir   or bld.env.PREFIX + 'share'
    opts.moduledir = opts.moduledir or bld.env.PREFIX + 'lib/ingen'

    # Headers
    for i in ['', 'client']:
        bld.install_files('${INCLUDEDIR}/ingen/%s' % i,
                          bld.path.ant_glob('ingen/%s/*' % i))

    # Python modules
    if bld.env.PYTHONDIR:
        bld.install_files('${PYTHONDIR}/', 'scripts/ingen.py')

    # Modules
    bld.recurse('src')
    bld.recurse('src/server')

    if bld.env.INGEN_BUILD_CLIENT:
        bld.recurse('src/client')

    if bld.env.INGEN_BUILD_GUI:
        bld.recurse('src/gui')

    # Program
    obj = bld(features     = 'c cxx cxxprogram',
              source       = 'src/ingen/ingen.cpp',
              target       = 'src/ingen/ingen',
              includes     = ['.'],
              use          = 'libingen',
              install_path = '${BINDIR}')
    autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV INGEN LV2')

    # Test program
    if bld.env.BUILD_TESTS:
        for i in ['ingen_test', 'ingen_bench']:
            obj = bld(features     = 'cxx cxxprogram',
                      source       = 'tests/%s.cpp' % i,
                      target       = 'tests/%s' % i,
                      includes     = ['.'],
                      use          = 'libingen_profiled',
                      install_path = '',
                      cxxflags     = bld.env.INGEN_TEST_CXXFLAGS,
                      linkflags    = bld.env.INGEN_TEST_LINKFLAGS)
            autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV INGEN LV2 SRATOM')

    bld.install_files('${DATADIR}/applications', 'src/ingen/ingen.desktop')
    bld.install_files('${BINDIR}', 'scripts/ingenish', chmod=Utils.O755)
    bld.install_files('${BINDIR}', 'scripts/ingenams', chmod=Utils.O755)

    # Code documentation
    autowaf.build_dox(bld, 'INGEN', INGEN_VERSION, top, out)

    # Ontology documentation
    if bld.env.DOCS:
        bld(rule='lv2specgen.py ${SRC} ${TGT} -i -p ingen --copy-style --list-email ingen@drobilla.net --list-page http://lists.drobilla.net/listinfo.cgi/ingen-drobilla.net',
            source = 'bundles/ingen.lv2/ingen.ttl',
            target = 'ingen.lv2/ingen.html')

    # Man page
    bld.install_files('${MANDIR}/man1', 'doc/ingen.1')

    # Icons
    icon_dir   = os.path.join(bld.env.DATADIR, 'icons', 'hicolor')
    icon_sizes = [16, 22, 24, 32, 48, 64, 128, 256]
    for s in icon_sizes:
        d = '%dx%d' % (s, s)
        bld.install_as(
            os.path.join(icon_dir, d, 'apps', 'ingen.png'),
            os.path.join('icons', d, 'ingen.png'))

    bld.install_as(
        os.path.join(icon_dir, 'scalable', 'apps', 'ingen.svg'),
        os.path.join('icons', 'scalable', 'ingen.svg'))

    bld.install_files('${LV2DIR}/ingen.lv2/',
                      bld.path.ant_glob('bundles/ingen.lv2/*'))

    # Install template graph bundles
    for c in ['Stereo', 'Mono']:
        for t in ['Effect', 'Instrument']:
            bundle = '%s%s.ingen' % (c, t)
            bld.install_files('${LV2DIR}/%s/' % bundle,
                              bld.path.ant_glob('bundles/%s/*' % bundle))

    bld.add_post_fun(autowaf.run_ldconfig)

def lint(ctx):
    subprocess.call('cpplint.py --filter=-whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-readability/todo,-build/namespaces,-whitespace/line_length,-runtime/rtti,-runtime/references,-whitespace/blank_line,-runtime/sizeof,-readability/streams,-whitespace/operators,-whitespace/parens,-build/include,-build/storage_class `find -name *.cpp -or -name *.hpp`', shell=True)

def upload_docs(ctx):
    import shutil

    # Ontology documentation
    os.system('rsync -avz -e ssh bundles/ingen.lv2/ingen.ttl drobilla@drobilla.net:~/drobilla.net/ns/')
    os.system('rsync -avz -e ssh build/ingen.lv2/ingen.html drobilla@drobilla.net:~/drobilla.net/ns/')
    os.system('rsync -avz -e ssh build/ingen.lv2/style.css drobilla@drobilla.net:~/drobilla.net/ns/')

    # Doxygen documentation
    os.system('rsync -ravz --delete -e ssh build/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/ingen/')


def test(ctx):
    import difflib
    import sys

    def test_file_equals(path1, path2):
        diff = list(difflib.unified_diff(open(path1).readlines(),
                                         open(path2).readlines(),
                                         path1,
                                         path2))
        autowaf.run_test(ctx, APPNAME, [path2, len(diff) != 0])
        if len(diff) > 0:
            for line in diff:
                sys.stdout.write(line)

    os.environ['PATH'] = 'tests' + os.pathsep + os.getenv('PATH')
    os.environ['LD_LIBRARY_PATH'] = os.path.join('src')
    os.environ['INGEN_MODULE_PATH'] = os.pathsep.join([
            os.path.join('src', 'server')])

    autowaf.pre_test(ctx, APPNAME, dirs=['.', 'src', 'tests'])
    autowaf.begin_tests(ctx, APPNAME)

    empty      = ctx.path.find_node('tests/empty.ingen')
    empty_path = os.path.join(empty.abspath(), 'main.ttl')
    for i in ctx.path.ant_glob('tests/*.ttl'):
        # Run test
        autowaf.run_test(ctx, APPNAME,
                         'ingen_test --load %s --execute %s' % (empty.abspath(), i.abspath()),
                         dirs=['.', 'src', 'tests'])

        # Check undo output for changes
        base = os.path.basename(i.abspath().replace('.ttl', ''))
        undone_path = base + '.undo.ingen/main.ttl'
        test_file_equals(empty_path, os.path.abspath(undone_path))

        # Check redo output for changes
        out_path = base + '.out.ingen/main.ttl'
        redone_path = base + '.redo.ingen/main.ttl'
        test_file_equals(out_path, os.path.abspath(redone_path))

    autowaf.end_tests(ctx, APPNAME)
    autowaf.post_test(ctx, APPNAME, dirs=['.', 'src', 'tests'],
                      remove=['/usr*'])
