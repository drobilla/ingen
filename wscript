#!/usr/bin/env python

import os
import subprocess

from waflib import Logs, Options, Utils
from waflib.extras import autowaf

# Package version
INGEN_VERSION       = '0.5.1'
INGEN_MAJOR_VERSION = '0'

# Mandatory waf variables
APPNAME = 'ingen'        # Package name for waf dist
VERSION = INGEN_VERSION  # Package version for waf dist
top     = '.'            # Source directory
out     = 'build'        # Build directory

line_just = 47

def options(ctx):
    ctx.load('compiler_cxx')
    ctx.load('python')
    ctx.load('lv2')
    ctx.recurse('src/gui')
    opt = ctx.configuration_options()

    opt.add_option('--data-dir', type='string', dest='datadir',
                   help='ingen data install directory [default: PREFIX/share/ingen]')
    opt.add_option('--module-dir', type='string', dest='moduledir',
                   help='ingen module install directory [default: PREFIX/lib/ingen]')

    ctx.add_flags(
        opt,
        {'no-gui':          'do not build GUI',
         'no-client':       'do not build client library (or GUI)',
         'no-jack':         'do not build jack backend (for ingen.lv2 only)',
         'no-plugin':       'do not build ingen.lv2 plugin',
         'no-python':       'do not install Python bindings',
         'no-webkit':       'do not use webkit to display plugin documentation',
         'no-jack-session': 'do not build JACK session support',
         'no-socket':       'do not build Socket interface',
         'debug-urids':     'print a trace of URI mapping',
         'portaudio':       'build PortAudio backend'})

def configure(conf):
    conf.load('compiler_cxx', cache=True)
    conf.load('lv2', cache=True)
    if not Options.options.no_python:
        conf.load('python', cache=True)

    conf.load('autowaf', cache=True)
    autowaf.set_cxx_lang(conf, 'c++11')

    conf.check_cxx(header_name='boost/intrusive/slist.hpp')
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
                      atleast_version='1.16.0', mandatory=True)
    autowaf.check_pkg(conf, 'lilv-0', uselib_store='LILV',
                      atleast_version='0.21.5', mandatory=True)
    autowaf.check_pkg(conf, 'suil-0', uselib_store='SUIL',
                      atleast_version='0.8.7', mandatory=True)
    autowaf.check_pkg(conf, 'sratom-0', uselib_store='SRATOM',
                      atleast_version='0.4.6', mandatory=True)
    autowaf.check_pkg(conf, 'raul', uselib_store='RAUL',
                      atleast_version='0.8.10', mandatory=True)
    autowaf.check_pkg(conf, 'serd-0', uselib_store='SERD',
                      atleast_version='0.30.0', mandatory=False)
    autowaf.check_pkg(conf, 'sord-0', uselib_store='SORD',
                      atleast_version='0.12.0', mandatory=False)
    autowaf.check_pkg(conf, 'portaudio-2.0', uselib_store='PORTAUDIO',
                      atleast_version='2.0.0', mandatory=False)

    autowaf.check_function(conf, 'cxx',  'posix_memalign',
                           defines     = '_POSIX_C_SOURCE=200809L',
                           header_name = 'stdlib.h',
                           define_name = 'HAVE_POSIX_MEMALIGN',
                           mandatory   = False)

    autowaf.check_function(conf, 'cxx',  'isatty',
                           header_name = 'unistd.h',
                           defines     = '_POSIX_C_SOURCE=200809L',
                           define_name = 'HAVE_ISATTY',
                           mandatory   = False)

    autowaf.check_function(conf, 'cxx',  'vasprintf',
                           header_name = 'stdio.h',
                           defines     = '_GNU_SOURCE=1',
                           define_name = 'HAVE_VASPRINTF',
                           mandatory   = False)

    conf.check(define_name = 'HAVE_LIBDL',
               lib         = 'dl',
               mandatory   = False)

    if not Options.options.no_socket:
        autowaf.check_function(conf, 'cxx',  'socket',
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
        autowaf.check_function(conf, 'cxx',  'jack_set_property',
                   header_name   = 'jack/metadata.h',
                   define_name   = 'HAVE_JACK_METADATA',
                   uselib        = 'JACK',
                   mandatory     = False)
        autowaf.check_function(conf, 'cxx',  'jack_port_rename',
                   header_name   = 'jack/jack.h',
                   define_name   = 'HAVE_JACK_PORT_RENAME',
                   uselib        = 'JACK',
                   mandatory     = False)
        if not Options.options.no_jack_session:
            autowaf.define(conf, 'INGEN_JACK_SESSION', 1)

    if Options.options.debug_urids:
        autowaf.define(conf, 'INGEN_DEBUG_URIDS', 1)

    conf.env.INGEN_TEST_LINKFLAGS = []
    conf.env.INGEN_TEST_CXXFLAGS  = []
    if conf.env.BUILD_TESTS:
        if not conf.env.NO_COVERAGE:
            conf.env.INGEN_TEST_CXXFLAGS  += ['--coverage']
            conf.env.INGEN_TEST_LINKFLAGS += ['--coverage']

    conf.env.PTHREAD_CFLAGS = []
    conf.env.PTHREAD_LINKFLAGS = []
    if conf.check(cflags=['-pthread'], mandatory=False):
        conf.env.PTHREAD_CFLAGS = ['-pthread']
    if conf.check(linkflags=['-pthread'], mandatory=False):
        if not (conf.env.DEST_OS == 'darwin' and conf.env.CXX_NAME == 'clang'):
            conf.env.PTHREAD_LINKFLAGS += ['-pthread']
    if conf.check(linkflags=['-lpthread'], mandatory=False):
        conf.env.PTHREAD_LINKFLAGS += ['-lpthread']

    conf.define('INGEN_SHARED', 1);
    conf.define('INGEN_VERSION', INGEN_VERSION)

    if not Options.options.no_client:
        autowaf.define(conf, 'INGEN_BUILD_CLIENT', 1)
    else:
        Options.options.no_gui = True

    if not Options.options.no_gui:
        conf.recurse('src/gui')

    if conf.env.HAVE_JACK:
        autowaf.define(conf, 'HAVE_JACK_MIDI', 1)

    autowaf.define(conf, 'INGEN_MAJOR_VERSION', INGEN_MAJOR_VERSION)
    autowaf.define(conf, 'INGEN_DATA_DIR',
                   os.path.join(conf.env.DATADIR, 'ingen'))
    autowaf.define(conf, 'INGEN_MODULE_DIR',
                   conf.env.LIBDIR)
    autowaf.define(conf, 'INGEN_BUNDLE_DIR',
                   os.path.join(conf.env.LV2DIR, 'ingen.lv2'))

    autowaf.set_lib_env(conf, 'ingen', INGEN_VERSION)
    conf.run_env.append_unique('XDG_DATA_DIRS', conf.build_path())
    for i in ['src', 'modules']:
        conf.run_env.append_unique(autowaf.lib_path_name, conf.build_path(i))
    for i in ['src/client', 'src/server', 'src/gui']:
        conf.run_env.append_unique('INGEN_MODULE_PATH', conf.build_path(i))

    conf.write_config_header('ingen_config.h', remove=False)

    autowaf.display_summary(
        conf,
        {'GUI':                     bool(conf.env.INGEN_BUILD_GUI),
         'HTML plugin doc support': bool(conf.env.HAVE_WEBKIT),
         'PortAudio driver':        bool(conf.env.HAVE_PORTAUDIO),
         'Jack driver':             bool(conf.env.HAVE_JACK),
         'Jack session support':    bool(conf.env.INGEN_JACK_SESSION),
         'Jack metadata support':   conf.is_defined('HAVE_JACK_METADATA'),
         'LV2 plugin driver':       bool(conf.env.INGEN_BUILD_LV2),
         'LV2 bundle':              conf.env.INGEN_BUNDLE_DIR,
         'LV2 plugin support':      bool(conf.env.HAVE_LILV),
         'Socket interface':        conf.is_defined('HAVE_SOCKET')})

unit_tests = ['tst_FilePath']

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
              target       = 'ingen',
              includes     = ['.'],
              use          = 'libingen',
              install_path = '${BINDIR}')
    autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV LV2')

    # Test program
    if bld.env.BUILD_TESTS:
        for i in ['ingen_test', 'ingen_bench'] + unit_tests:
            obj = bld(features     = 'cxx cxxprogram',
                      source       = 'tests/%s.cpp' % i,
                      target       = 'tests/%s' % i,
                      includes     = ['.'],
                      use          = 'libingen',
                      install_path = '',
                      cxxflags     = bld.env.INGEN_TEST_CXXFLAGS,
                      linkflags    = bld.env.INGEN_TEST_LINKFLAGS)
            autowaf.use_lib(bld, obj, 'GTHREAD GLIBMM SORD RAUL LILV LV2 SRATOM')

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
    "checks code for style issues"
    import subprocess
    cmd = ("clang-tidy -p=. -header-filter=ingen/ -checks=\"*," +
           "-clang-analyzer-alpha.*," +
           "-cppcoreguidelines-*," +
           "-cppcoreguidelines-pro-type-union-access," +
           "-google-build-using-namespace," +
           "-google-readability-casting," +
           "-google-readability-todo," +
           "-llvm-header-guard," +
           "-llvm-include-order," +
           "-llvm-namespace-comment," +
           "-misc-unused-parameters," +
           "-readability-else-after-return," +
           "-readability-implicit-bool-cast," +
           "-readability-named-parameter\" " +
           "$(find .. -name '*.cpp')")
    subprocess.call(cmd, cwd='build', shell=True)

def upload_docs(ctx):
    import shutil

    # Ontology documentation
    os.system('rsync -avz -e ssh bundles/ingen.lv2/ingen.ttl drobilla@drobilla.net:~/drobilla.net/ns/')
    os.system('rsync -avz -e ssh build/ingen.lv2/ingen.html drobilla@drobilla.net:~/drobilla.net/ns/')
    os.system('rsync -avz -e ssh build/ingen.lv2/style.css drobilla@drobilla.net:~/drobilla.net/ns/')

    # Doxygen documentation
    os.system('rsync -ravz --delete -e ssh build/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/ingen/')

def test(tst):
    with tst.group('unit') as check:
        for i in unit_tests:
            check(['./tests/' + i])

    with tst.group('integration') as check:
        empty      = tst.src_path('tests/empty.ingen')
        empty_main = os.path.join(empty, 'main.ttl')
        for i in tst.path.ant_glob('tests/*.ttl'):
            base = os.path.basename(i.abspath().replace('.ttl', ''))

            # Run test
            check(['./tests/ingen_test',
                   '--load', empty,
                   '--execute', os.path.relpath(i.abspath(), os.getcwd())])

            # Check undo output for changes
            check.file_equals(empty_main, base + '.undo.ingen/main.ttl')

            # Check redo output for changes
            check.file_equals(base + '.out.ingen/main.ttl',
                              base + '.redo.ingen/main.ttl')
