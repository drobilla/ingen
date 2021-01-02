#!/usr/bin/env python

import os

from waflib import Build, Logs, Options, Utils
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
        {'no-gui': 'do not build GUI',
         'no-client': 'do not build client library (or GUI)',
         'no-jack': 'do not build jack backend (for ingen.lv2 only)',
         'no-plugin': 'do not build ingen.lv2 plugin',
         'no-python': 'do not install Python bindings',
         'no-webkit': 'do not use webkit to display plugin documentation',
         'no-socket': 'do not build Socket interface',
         'debug-urids': 'print a trace of URI mapping',
         'portaudio': 'build PortAudio backend'})


def configure(conf):
    conf.load('compiler_cxx', cache=True)
    conf.load('lv2', cache=True)
    if not Options.options.no_python:
        conf.load('python', cache=True)

    conf.load('autowaf', cache=True)
    autowaf.set_cxx_lang(conf, 'c++11')

    if Options.options.strict:
        # Check for programs used by lint target
        conf.find_program("flake8", var="FLAKE8", mandatory=False)
        conf.find_program("clang-tidy", var="CLANG_TIDY", mandatory=False)
        conf.find_program("iwyu_tool", var="IWYU_TOOL", mandatory=False)

    if Options.options.ultra_strict:
        autowaf.add_compiler_flags(conf.env, 'cxx', {
            'clang': [
                '-Wno-cast-align',
                '-Wno-cast-qual',
                '-Wno-documentation-unknown-command',
                '-Wno-exit-time-destructors',
                '-Wno-float-conversion',
                '-Wno-float-equal',
                '-Wno-format-nonliteral',
                '-Wno-global-constructors',
                '-Wno-implicit-float-conversion',
                '-Wno-implicit-int-conversion',
                '-Wno-nullability-extension',
                '-Wno-nullable-to-nonnull-conversion',
                '-Wno-padded',
                '-Wno-reserved-id-macro',
                '-Wno-shadow-field',
                '-Wno-shorten-64-to-32',
                '-Wno-sign-conversion',
                '-Wno-switch-enum',
                '-Wno-unreachable-code',
                '-Wno-unused-parameter',
                '-Wno-vla',
                '-Wno-vla-extension',
                '-Wno-weak-vtables',
            ],
            'gcc': [
                '-Wno-cast-align',
                '-Wno-cast-qual',
                '-Wno-conditionally-supported',
                '-Wno-conversion',
                '-Wno-effc++',
                '-Wno-float-conversion',
                '-Wno-float-equal',
                '-Wno-format',
                '-Wno-format-nonliteral',
                '-Wno-multiple-inheritance',
                '-Wno-padded',
                '-Wno-sign-conversion',
                '-Wno-stack-protector',
                '-Wno-suggest-attribute=format',
                '-Wno-suggest-override',
                '-Wno-switch-enum',
                '-Wno-unreachable-code',
                '-Wno-unused-parameter',
                '-Wno-useless-cast',
                '-Wno-vla',
            ],
        })

    conf.check_cxx(header_name='boost/intrusive/slist.hpp')

    conf.check_pkg('lv2 >= 1.16.0', uselib_store='LV2')
    conf.check_pkg('lilv-0 >= 0.21.5', uselib_store='LILV')
    conf.check_pkg('suil-0 >= 0.8.7', uselib_store='SUIL')
    conf.check_pkg('sratom-0 >= 0.4.6', uselib_store='SRATOM')
    conf.check_pkg('raul-1 >= 1.1.0', uselib_store='RAUL')
    conf.check_pkg('serd-0 >= 0.30.3', uselib_store='SERD', mandatory=False)
    conf.check_pkg('sord-0 >= 0.12.0', uselib_store='SORD', mandatory=False)

    conf.check_pkg('portaudio-2.0',
                   uselib_store = 'PORTAUDIO',
                   system       = True,
                   mandatory    = False)

    conf.check_pkg('sigc++-2.0',
                   uselib_store = 'SIGCPP',
                   system       = True,
                   mandatory    = False)

    conf.check_function('cxx', 'posix_memalign',
                        defines     = '_POSIX_C_SOURCE=200809L',
                        header_name = 'stdlib.h',
                        define_name = 'HAVE_POSIX_MEMALIGN',
                        return_type = 'int',
                        arg_types   = 'void**,size_t,size_t',
                        mandatory   = False)

    conf.check_function('cxx', 'isatty',
                        header_name = 'unistd.h',
                        defines     = '_POSIX_C_SOURCE=200809L',
                        define_name = 'HAVE_ISATTY',
                        return_type = 'int',
                        arg_types   = 'int',
                        mandatory   = False)

    conf.check_function('cxx', 'vasprintf',
                        header_name = 'stdio.h',
                        defines     = '_GNU_SOURCE=1',
                        define_name = 'HAVE_VASPRINTF',
                        return_type = 'int',
                        arg_types   = 'char**,const char*,va_list',
                        mandatory   = False)

    conf.check(define_name = 'HAVE_LIBDL',
               lib         = 'dl',
               mandatory   = False)

    if not Options.options.no_socket:
        conf.check_function('cxx', 'socket',
                            header_name = 'sys/socket.h',
                            define_name = 'HAVE_SOCKET',
                            return_type = 'int',
                            arg_types   = 'int,int,int',
                            mandatory   = False)

    if not Options.options.no_python:
        conf.check_python_version((2, 4, 0), mandatory=False)

    if not Options.options.no_plugin:
        conf.env.INGEN_BUILD_LV2 = 1

    if not Options.options.no_jack:
        conf.check_pkg('jack >= 0.120.0',
                       uselib_store = 'JACK',
                       system       = True,
                       mandatory    = False)

        conf.check_function('cxx', 'jack_set_property',
                            header_name   = 'jack/metadata.h',
                            define_name   = 'HAVE_JACK_METADATA',
                            uselib        = 'JACK',
                            return_type   = 'int',
                            arg_types     = '''jack_client_t*,
                                               jack_uuid_t,
                                               const char*,
                                               const char*,
                                               const char*''',
                            mandatory     = False)
        conf.check_function('cxx', 'jack_port_rename',
                            header_name   = 'jack/jack.h',
                            define_name   = 'HAVE_JACK_PORT_RENAME',
                            uselib        = 'JACK',
                            return_type   = 'int',
                            arg_types     = 'jack_client_t*,jack_port_t*,const char*',
                            mandatory     = False)

    if Options.options.debug_urids:
        conf.define('INGEN_DEBUG_URIDS', 1)

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

    conf.define('INGEN_SHARED', 1)
    conf.define('INGEN_VERSION', INGEN_VERSION)

    if conf.env.HAVE_SIGCPP and not Options.options.no_client:
        conf.env.INGEN_BUILD_CLIENT = 1
    else:
        Options.options.no_gui = True

    if not Options.options.no_gui:
        conf.recurse('src/gui')

    conf.env.INGEN_MAJOR_VERSION = INGEN_MAJOR_VERSION

    conf.define('INGEN_DATA_DIR', os.path.join(conf.env.DATADIR, 'ingen'))
    conf.define('INGEN_MODULE_DIR', conf.env.LIBDIR)
    conf.define('INGEN_BUNDLE_DIR', os.path.join(conf.env.LV2DIR, 'ingen.lv2'))

    autowaf.set_lib_env(conf, 'ingen', INGEN_VERSION)
    conf.run_env.append_unique('XDG_DATA_DIRS', [str(conf.path.get_bld())])
    for i in ['src', 'modules']:
        conf.run_env.append_unique(autowaf.lib_path_name, [str(conf.build_path(i))])
    for i in ['src/client', 'src/server', 'src/gui']:
        conf.run_env.append_unique('INGEN_MODULE_PATH', [str(conf.build_path(i))])

    conf.write_config_header('ingen_config.h', remove=False)

    autowaf.display_summary(
        conf,
        {'GUI': bool(conf.env.INGEN_BUILD_GUI),
         'HTML plugin doc support': bool(conf.env.HAVE_WEBKIT),
         'PortAudio driver': bool(conf.env.HAVE_PORTAUDIO),
         'Jack driver': bool(conf.env.HAVE_JACK),
         'Jack metadata support': conf.is_defined('HAVE_JACK_METADATA'),
         'LV2 plugin driver': bool(conf.env.INGEN_BUILD_LV2),
         'LV2 bundle': conf.env.INGEN_BUNDLE_DIR,
         'LV2 plugin support': bool(conf.env.HAVE_LILV),
         'Socket interface': conf.is_defined('HAVE_SOCKET')})


unit_tests = ['tst_FilePath']


def build(bld):
    opts           = Options.options
    opts.datadir   = opts.datadir or bld.env.PREFIX + 'share'
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
    bld(features     = 'c cxx cxxprogram',
        source       = 'src/ingen/ingen.cpp',
        target       = 'ingen',
        includes     = ['.', 'include'],
        use          = 'libingen',
        uselib       = 'SERD SORD SRATOM RAUL LILV LV2',
        install_path = '${BINDIR}')

    # Test program
    if bld.env.BUILD_TESTS:
        for i in ['ingen_test', 'ingen_bench'] + unit_tests:
            bld(features     = 'cxx cxxprogram',
                source       = 'tests/%s.cpp' % i,
                target       = 'tests/%s' % i,
                includes     = ['.', 'include'],
                use          = 'libingen',
                uselib       = 'SERD SORD SRATOM RAUL LILV LV2',
                install_path = '',
                cxxflags     = bld.env.INGEN_TEST_CXXFLAGS,
                linkflags    = bld.env.INGEN_TEST_LINKFLAGS)

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


class LintContext(Build.BuildContext):
    fun = cmd = 'lint'


def lint(ctx):
    "checks code for style issues"
    import subprocess
    import sys

    st = 0

    if "FLAKE8" in ctx.env:
        Logs.info("Running flake8")
        for i in ["src/client/wscript",
                  "src/gui/wscript",
                  "src/server/wscript",
                  "src/wscript",
                  "scripts/ingen.py",
                  "scripts/ingenish",
                  "scripts/ingenams",
                  "wscript"]:
            st += subprocess.call([ctx.env.FLAKE8[0],
                                   "--ignore", "E221,W504,E251,E501",
                                   i])
    else:
        Logs.warn("Not running flake8")

    if "CLANG_TIDY" in ctx.env and "clang" in ctx.env.CXX[0]:
        Logs.info("Running clang-tidy")

        import json

        with open('build/compile_commands.json', 'r') as db:
            commands = json.load(db)
            files = [c['file'] for c in commands]

        for step_files in zip(*(iter(files),) * Options.options.jobs):
            procs = []
            for f in step_files:
                cmd = [ctx.env.CLANG_TIDY[0], '--quiet', '-p=.', f]
                procs += [subprocess.Popen(cmd, cwd='build')]

            for proc in procs:
                proc.communicate()
                st += proc.returncode
    else:
        Logs.warn("Not running clang-tidy")

    if "IWYU_TOOL" in ctx.env:
        Logs.info("Running include-what-you-use")
        cmd = [ctx.env.IWYU_TOOL[0], "-o", "clang", "-p", "build"]
        output = subprocess.check_output(cmd).decode('utf-8')
        if 'error: ' in output:
            sys.stdout.write(output)
            st += 1
    else:
        Logs.warn("Not running include-what-you-use")

    if st != 0:
        Logs.warn("Lint checks failed")
        sys.exit(st)


def upload_docs(ctx):
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
