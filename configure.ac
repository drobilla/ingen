AC_PREREQ(2.59)
AC_INIT([ingen],[svn],[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])


################### SOURCES

# Ingen
AC_CONFIG_SRCDIR([src/common/interface/EngineInterface.hpp])
AC_CONFIG_SRCDIR([src/common/lv2ext/lv2-miditype.h])
AC_CONFIG_SRCDIR([src/libs/module/Module.cpp])
AC_CONFIG_SRCDIR([src/libs/engine/JackAudioDriver.cpp])
AC_CONFIG_SRCDIR([src/libs/serialisation/Loader.cpp])
AC_CONFIG_SRCDIR([src/libs/client/OSCController.cpp])
AC_CONFIG_SRCDIR([src/libs/gui/App.cpp])
AC_CONFIG_SRCDIR([src/progs/ingen/main.cpp])

AC_CONFIG_HEADER([config/config.h])

AM_INIT_AUTOMAKE

#################### ENVIRONMENT

AC_LANG([C++])

AC_PROG_CXX
AM_PROG_CC_C_O

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

# Check for boost smart pointers
AC_CHECK_HEADERS([boost/shared_ptr.hpp], [],  
	AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))
AC_CHECK_HEADERS([boost/weak_ptr.hpp], [],  
	AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))

# This is cute... autohell FTW
builddir=`pwd`
cd $srcdir
abs_srcdir=`pwd`
cd $builddir

PKG_CHECK_MODULES([SLV2], [libslv2])
PKG_CHECK_MODULES([RAUL], [raul])
PKG_CHECK_MODULES([FLOWCANVAS], [flowcanvas])

CONFIG_H_PATH="$builddir/config/config.h"
	
INGEN_CFLAGS="-I$abs_srcdir/src/common -I$abs_srcdir/src/libs -I$abs_srcdir/src/libs/engine/events -I$abs_srcdir/src"
AC_SUBST(INGEN_CFLAGS)

#################### COMMAND LINE PARAMETERS

AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging symbols and assertions (no)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  debug_symbols="yes"
  debug_assertions="yes"
fi

AC_ARG_ENABLE(debug_symbols,
	[AS_HELP_STRING(--enable-debug-symbols, [Enable debugging symbols - overrides CXXFLAGS (no)])],
	[debug_symbols="$enableval"])

AC_ARG_ENABLE(debug_asertions,
	[AS_HELP_STRING(--enable-debug-assertions, [Enable debugging assertions (no)])],
	[assertions="$enableval"])

if test "$debug_symbols" = "yes"; then
  CFLAGS="-O0 -g"
  CXXFLAGS="-O0 -g"
fi

if test "$debug_assertions" = "yes"; then
  CFLAGS="$CFLAGS -DDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DDEBUG -DLIBRDF_DEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG -DLIBRDF_DEBUG"
fi

# Boost shared_ptr debugging
pointer_debug="no"
AC_ARG_ENABLE(pointer_debug,
	[AS_HELP_STRING(--enable-pointer-debug, [Smart pointer debugging (DEVELOPERS ONLY) (no)])],
	[pointer_debug="$enableval"])
if test "$pointer_debug" = "yes"; then
  CFLAGS="$CFLAGS -DBOOST_SP_ENABLE_DEBUG_HOOKS"
  CXXFLAGS="$CXXFLAGS -DBOOST_SP_ENABLE_DEBUG_HOOKS"
fi

# Use strict flags?
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (no)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  # Stupid Gtkmm won't build with -pedantic
  CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Winit-self"
  CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wno-unused-parameter -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

# Bolt on a few specific flags to CFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -ansi -pipe -fmessage-length=999 -DCONFIG_H_PATH=\\\"$CONFIG_H_PATH\\\""
CFLAGS="$CFLAGS -std=c99 -pipe -fmessage-length=999 -DCONFIG_H_PATH=\\\"$CONFIG_H_PATH\\\""


#################### GENERIC OPTIONS


# Jack support
build_jack="yes"
AC_ARG_ENABLE(enable-jack,
	[AS_HELP_STRING(--enable-jack, [Enable Jack support (yes)])],
	[ if test x$enable_jack = xno ; then build_jack=no ; fi ])
if test "$build_jack" = "yes"; then
	PKG_CHECK_MODULES(JACK, jack >= 0.107.0)
	AC_DEFINE(HAVE_JACK, 1, [Has Jack])
	AC_DEFINE(HAVE_JACK_MIDI, 1, [Has Jack MIDI])
fi

# LV2 support
build_lv2="yes"
AC_ARG_ENABLE(lv2,
	[AS_HELP_STRING(--enable-lv2, [Enable LV2 plugin support through libslv2 (yes) - Requires: libslv2])],
	[ if test x$enable_lv2 = xno ; then build_lv2=no ; fi ])
if test "$build_lv2" = "yes"; then
	# Configure SLV2 (independent build system)
#	AC_CONFIG_SUBDIRS([slv2])
	AC_DEFINE(HAVE_SLV2, 1, [Has libslv2])
fi
	
# Build LV2 GTK GUI
build_lv2_gui="yes"
AC_ARG_ENABLE(lv2-gui,
	[AS_HELP_STRING(--enable-lv2-gui, [Build plugin GUIs (requires Gtk2)])],
	[ if test x$enable_lv2_gui = xno ; then build_lv2_gui=no ; fi ])
if test "$build_lv2_gui" = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0)
fi
AM_CONDITIONAL(WITH_LV2_GUI, [test "$build_lv2_gui" = "yes"])


# LADSPA support
build_ladspa="yes"
AC_ARG_ENABLE(ladspa,
	[AS_HELP_STRING(--enable-ladspa, [Enable LADSPA plugin support (yes)])],
	[ if test x$enable_ladspa = xno ; then build_ladspa=no ; fi ])
if test "$build_ladspa" = "yes"; then
	build_ladspa="no"
	AC_CHECK_HEADER([ladspa.h], [build_ladspa="yes"],
		[AC_MSG_WARN([You don't seem to build ladspa.h, Ingen will not be very useful!])])
fi
if test "$build_ladspa" = "yes"; then
	AC_DEFINE(HAVE_LADSPA, 1, [Has ladspa.h])
fi


# LASH support
build_lash="yes"
AC_ARG_ENABLE(lash,
	[AS_HELP_STRING(--enable-lash, [Enable LASH session management support (yes)])],
	[ if test x$enable_lash = xno ; then build_lash=no ; fi ])
	
have_lash="no"
if test "$build_lash" = "yes"; then
	PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.2, have_lash="yes", have_lash="no")
fi
if test "$have_lash" = "yes"; then
	AC_DEFINE(HAVE_LASH, 1, [Has lash.h])
else
	AC_MSG_WARN([LASH not found, session support will not be built.])
fi

# SWIG bindings
bindings="no"
AC_ARG_ENABLE(bindings,
	[AS_HELP_STRING(--enable-bindings, [Build language bindings via SWIG (false)])],
	[bindings="$enableval"])
if test "$bindings" = "yes"; then
  AC_CHECK_PROGS(SWIG, swig)
fi
AM_CONDITIONAL(WITH_SWIG, [test "$bindings" = "yes"])
AC_DEFINE(WITH_BINDINGS, 1, [Building with SWIG language bindings])

AM_CONDITIONAL(WITH_LASH, [test "$have_lash" = "yes"])
AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])
AM_CONDITIONAL(BUILD_SERVER, [test "$build_ingen_server" = "yes"])
AM_CONDITIONAL(BUILD_IN_PROCESS_ENGINE, [test "$build_in_process_engine" = "yes"])
AM_CONDITIONAL(BUILD_UNIT_TESTS, [test "$build_unit_tests" = "yes"])
AM_CONDITIONAL(WITH_LADSPA, [test "$build_ladspa" = "yes"])
AM_CONDITIONAL(WITH_LV2, [test "$build_lv2" = "yes"])


################# INGEN OPTIONS

# Build support for running as an in-process jack client
build_in_process_engine="no"
AC_ARG_ENABLE([in-process-engine],
	AS_HELP_STRING(--enable-in-process-engine, Build engine as an in-process Jack client (no)),
	[ if test x$enable_in_process_engine = xyes ; then build_in_process_engine=yes ; fi ])

if test "$build_in_process_engine" = "yes"; then
	AC_DEFINE(BUILD_IN_PROCESS_ENGINE, 1, [Whether to support running as a Jack in-process client])
fi

# Stand-alone engine (OSC controlled)
build_ingen_server="yes"
AC_ARG_ENABLE([ingen-server],
	AS_HELP_STRING(--enable-server, Build OSC controlled stand-alone engine (yes)),
	[ if test x$enable_server = xno ; then build_ingen_server=no ; fi ])

# Command-line clients
build_ingen_client_lib="yes"
AC_ARG_ENABLE([ingen-clients],
	AS_HELP_STRING(--enable-ingen-clients, [Build Ingen client library (yes) - Requires: libxml2, raptor, libsigc++]),
	[ if test x$enable_ingen_clients = xno ; then build_ingen_client_lib=no ; fi ])

# Gtk client (Ingenuity)
build_ingen_gtk_client="yes"
AC_ARG_ENABLE([ingen-gtk-client],
	AS_HELP_STRING(--enable-ingen-gtk-client, [Build Ingen GTK client (true)]),
	[ if test x$enable_gtk_client = xno ; then build_gtk_client=no ; fi ])

if test "$build_ingen_gtk_client" = "yes"; then
	build_ingen_clients="yes"
fi

# Clients
if test "$build_ingen_client_lib" = "yes"; then
	AC_CHECK_HEADERS([sys/time.h unistd.h])
	PKG_CHECK_MODULES(LXML2, libxml-2.0 >= 2.6.0) # FIXME: deprecated, make optional
	PKG_CHECK_MODULES(LSIGCPP, sigc++-2.0)
	PKG_CHECK_MODULES(LIBLO, liblo)
	PKG_CHECK_MODULES(REDLANDMM, [redlandmm])
fi
AM_CONDITIONAL(WITH_REDLANDMM, [test "$build_redlandmm" = "yes"])

else
 	AC_MSG_WARN([Ingen client library/executable will not be built!])
fi
AM_CONDITIONAL(BUILD_INGEN_CLIENTS, [test "$build_ingen_clients" = "yes"])

if test "$build_ingen_gtk_client" = "yes"; then
#	AC_CHECK_HEADERS([sys/time.h unistd.h])
#	AC_CHECK_HEADERS([string.h sys/time.h unistd.h])
#	AC_HEADER_TIME
#	AC_FUNC_ERROR_AT_LINE
#	AC_FUNC_FORK
#	AC_FUNC_MALLOC
#	AC_FUNC_STAT
#	AC_CHECK_FUNCS([gettimeofday mkdir strcasecmp strchr strdup strtol])

	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
	PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
	PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
	#PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
	PKG_CHECK_MODULES(CURL, libcurl >= 7.15.0, build_curl="yes", build_curl="no")
	if test "$build_curl" = "yes"; then
		AC_DEFINE(HAVE_CURL, 1, [Has the libcurl library])
	fi
else
  AC_MSG_WARN([GTK client will NOT be built!])
fi
AM_CONDITIONAL(BUILD_GTK_CLIENT, [test "$build_ingen_gtk_client" = "yes"])
AM_CONDITIONAL(WITH_CURL, [test "$build_curl" = "yes"])

# Build ingen client lib if anything above that depends on it is to be built
AM_CONDITIONAL(BUILD_CLIENT_LIB, [test "$build_ingen_clients" = "yes" -o "$build_ingen_gtk_client" = "yes"])


#################### OUTPUT

# Ingen
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/common/Makefile])
AC_CONFIG_FILES([src/common/interface/Makefile])
AC_CONFIG_FILES([src/common/lv2ext/Makefile])
AC_CONFIG_FILES([src/libs/Makefile])
AC_CONFIG_FILES([src/libs/module/Makefile])
AC_CONFIG_FILES([src/libs/engine/Makefile])
AC_CONFIG_FILES([src/libs/engine/events/Makefile])
AC_CONFIG_FILES([src/libs/serialisation/Makefile])
AC_CONFIG_FILES([src/libs/client/Makefile])
AC_CONFIG_FILES([src/libs/gui/Makefile])
AC_CONFIG_FILES([src/bindings/Makefile])
AC_CONFIG_FILES([src/progs/Makefile])
AC_CONFIG_FILES([src/progs/ingen/Makefile])
AC_CONFIG_FILES([src/progs/supercollider/Makefile])
AC_CONFIG_FILES([src/progs/python/Makefile])
AC_CONFIG_FILES([src/progs/python/scripts/Makefile])
AC_CONFIG_FILES([src/progs/ingen/ingen.desktop])
AC_CONFIG_FILES([patches/Makefile])
AC_CONFIG_FILES([doc/Doxyfile])


AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Jack support:            $build_jack])
AC_MSG_RESULT([LV2 Plugin support:      $build_lv2])
AC_MSG_RESULT([LADSPA Plugin support:   $build_ladspa])
AC_MSG_RESULT([])
AC_MSG_RESULT([Ingen:])
AC_MSG_RESULT([    Engine library/OSC server: $build_ingen_server])
AC_MSG_RESULT([    JACK in-process engine:    $build_in_process_engine])
AC_MSG_RESULT([    Client library:            $build_ingen_client_lib])
AC_MSG_RESULT([    GTK GUI:                   $build_ingen_gtk_client])
AC_MSG_RESULT([])
AC_MSG_RESULT([*** INGEN IS WITHOUT LASH SUPPORT AT THIS TIME ***])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([Please note the above and see README for performance information.])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

