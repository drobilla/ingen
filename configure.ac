AC_PREREQ(2.59)
AC_INIT([ingen],[0.4.0pre],[dave@drobilla.net])
AC_CONFIG_SRCDIR([src/common/interface/EngineInterface.h])
AC_CONFIG_SRCDIR([src/common/lv2ext/lv2-miditype.h])
AC_CONFIG_SRCDIR([src/libs/module/Module.cpp])
AC_CONFIG_SRCDIR([src/libs/engine/JackAudioDriver.cpp])
AC_CONFIG_SRCDIR([src/libs/engine/tests/node_tree_test.cpp])
AC_CONFIG_SRCDIR([src/libs/serialisation/Loader.cpp])
AC_CONFIG_SRCDIR([src/libs/client/OSCController.cpp])
AC_CONFIG_SRCDIR([src/libs/gui/App.cpp])
AC_CONFIG_SRCDIR([src/progs/ingen/main.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE


##########################################################################
## Global Checks
##########################################################################

# Checks for compilers
AC_PROG_CXX
AC_PROG_LIBTOOL

# Check random other stuff
AC_C_CONST
AC_C_INLINE
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_FUNC_ERROR_AT_LINE
AC_HEADER_DIRENT

# Check for liblo
PKG_CHECK_MODULES(LOSC, liblo >= 0.22)

# Check for pthreads
AC_CHECK_LIB(pthread, pthread_create, [],
	AC_MSG_ERROR([*** Ingen requires POSIX threads support]))
AC_CHECK_HEADER([pthread.h])

PKG_CHECK_MODULES(RAUL, raul >= 0.0.0)

# Check for LASH
build_lash="no"
AC_ARG_ENABLE(lash,
	[AS_HELP_STRING(--enable-lash, [Enable LASH session management support (no)])],
	[ if test x$enable_lash = xno ; then build_lash=no ; fi ])
if test "$build_lash" = "yes"; then
	PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.0, build_lash="yes", build_lash="no")
fi
if test "$build_lash" = "yes"; then
	AC_DEFINE(HAVE_LASH, 1, [Has lash.h])
else
	AC_MSG_WARN([LASH not found, session support will not be built.])
fi
AC_MSG_WARN([NOTE: LASH support is not available at this time])

AM_CONDITIONAL(WITH_LASH, [test "$build_lash" = "yes"])


# Debug stuff


debug="no"
debug_symbols="no"
assertions="no"

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
  CXXFLAGS="$CFLAGS"
fi

if test "$debug_assertions" = "yes"; then
  CFLAGS="$CFLAGS -DDEBUG"
  CXXFLAGS="$CXXFLAGS -DDEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

# Boost shared_ptr debugging
pointer_debug="no"
AC_ARG_ENABLE(pointer_debug,
	[AS_HELP_STRING(--enable-pointer-debug, [Enable smart pointer debugging (no)])],
	[pointer_debug="$enableval"])
if test "$pointer_debug" = "yes"; then
  CFLAGS+=" -DBOOST_SP_ENABLE_DEBUG_HOOKS"
  CXXFLAGS+=" -DBOOST_SP_ENABLE_DEBUG_HOOKS"
fi

# Use strict flags?
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (no)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  # Stupid Gtkmm won't build with -pedantic
  CFLAGS="$CFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self"
  CXXFLAGS="$CXXFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

# Bolt on a few specific flags to CFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -fmessage-length=999"
CFLAGS="$CFLAGS -pipe -fmessage-length=999"



##########################################################################
## ENGINE (Universal engine options)
##########################################################################

# Build support for running as an in-process jack client
build_in_process_engine="no"
AC_ARG_ENABLE([in-process-engine],
	AS_HELP_STRING(--enable-in-process-engine, Build engine as an in-process Jack client (no)),
	[ if test x$enable_in_process_engine = xyes ; then build_in_process_engine=yes ; fi ])

# Jack MIDI support
build_jack_midi="no"
AC_ARG_ENABLE(jack-midi,
	[AS_HELP_STRING(--enable-jack-midi, [Enable Jack MIDI support (no)])],
	[ if test x$enable_jack_midi = xyes ; then build_jack_midi=yes ; fi ])

# ALSA support
build_alsa_midi="yes"
AC_ARG_ENABLE(alsa-midi,
	[AS_HELP_STRING(--enable-alsa-midi, [Enable Alsa MIDI driver (yes)])],
	[ if test x$enable_alsa_midi = xno ; then build_alsa_midi=no ; fi ])

# LV2 support
build_lv2="yes"
AC_ARG_ENABLE(lv2,
	[AS_HELP_STRING(--enable-lv2, [Enable LV2 plugin support through libslv2 (yes) - Requires: libslv2])],
	[ if test x$enable_lv2 = xno ; then build_lv2=no ; fi ])

# DSSI support
build_dssi="yes"
AC_ARG_ENABLE(dssi,
	[AS_HELP_STRING(--enable-dssi, [Enable DSSI plugin support (yes) - Requires: alsa])],
	[ if test x$enable_dssi = xno ; then build_dssi=no ; fi ])

# LADSPA support
build_ladspa="yes"
AC_ARG_ENABLE(ladspa,
	[AS_HELP_STRING(--enable-ladspa, [Enable LADSPA plugin support (yes)])],
	[ if test x$enable_ladspa = xno ; then build_ladspa=no ; fi ])


if test "$build_in_process_engine" = "yes"; then
	AC_DEFINE(BUILD_IN_PROCESS_ENGINE, 1, [Whether to support running as a Jack in-process client])
fi



##########################################################################
## COMPONENTS (options for which components to build)
##########################################################################

# Build unit tests?
build_unit_tests="no"
AC_ARG_ENABLE(unit-tests,
	[AS_HELP_STRING(--enable-unit-tests, [Build unit tests (no) - Developers only])],
	[build_unit_tests="$enableval"])

# Stand-alone engine (OSC controlled)
build_server="yes"
AC_ARG_ENABLE([server],
	AS_HELP_STRING(--enable-server, Build OSC controlled stand-alone engine (yes)),
	[ if test x$enable_server = xno ; then build_server=no ; fi ])

# Command-line clients
build_console_clients="yes"
AC_ARG_ENABLE([console-clients],
	AS_HELP_STRING(--enable-console-clients, [Build command-line clients (yes) - Requires: libxml2, raptor, libsigc++]),
	[ if test x$enable_console_clients = xno ; then build_console_clients=no ; fi ])

# Gtk client (Ingenuity)
build_gtk_client="yes"
AC_ARG_ENABLE([gtk-client],
	AS_HELP_STRING(--enable-gtk-client, [Build GTK client, Ingenuity (true)]),
	[ if test x$enable_gtk_client = xno ; then build_gtk_client=no ; fi ])


##########################################################################
## SERVER
##########################################################################

if test "$build_server" = "yes"; then
	# Check for standard lib stuff
	AC_CHECK_FUNCS([posix_memalign, fesetround])
	AC_CHECK_HEADERS([float.h stddef.h fenv.h])
	
	# Check random other stuff
	AC_FUNC_CLOSEDIR_VOID
	
	# Check for Jack
	# (102.20 = MIDI API changes)
	PKG_CHECK_MODULES(JACK, jack >= 0.103.0)

	# Check for Jack MIDI
	if test "$build_jack_midi" = "yes"; then
		AC_CHECK_HEADER([jack/midiport.h], [build_jack_midi="yes"],
			[AC_MSG_WARN([You don't seem to build jack/midiport.h, Jack MIDI support will not be built])])
	fi
	if test "$build_jack_midi" = "yes"; then
		AC_DEFINE(HAVE_JACK_MIDI, 1, [Has Jack MIDI])
		AC_JACK_MIDI_NFRAMES_CHECK()  	   	
		if test "$jackmidi_nframes" == "yes"; then 	  	
			AC_DEFINE([JACK_MIDI_NEEDS_NFRAMES], 1, [Defined if we JACK MIDI functions need nframes parameter.]) 	  	
		fi
	fi
	
	# Check for ALSA
	if test "$build_alsa_midi" = "yes" -o "$build_dssi" = "yes"; then
		PKG_CHECK_MODULES(ALSA, alsa >= 1.0.0, [have_alsa_midi="yes"], [have_alsa_midi="no"])
	fi

	if test "$build_alsa_midi" = "yes"; then
		if test "$build_jack_midi" = "yes"; then
			AC_MSG_WARN(["Only one MIDI driver can be built (for now).  Alsa MIDI disabled."])
			build_alsa_midi="no"
		fi
	fi
	if test "$build_alsa_midi" = "yes"; then
		AC_DEFINE(HAVE_ALSA_MIDI, 1, [Has Alsa MIDI])
	fi
	
	# Check for LADSPA
	if test "$build_ladspa" = "yes"; then
		build_ladspa="no"
		AC_CHECK_HEADER([ladspa.h], [build_ladspa="yes"],
			[AC_MSG_WARN([You don't seem to build ladspa.h, Ingen will not be very useful!])])
	fi
	if test "$build_ladspa" = "yes"; then
		AC_DEFINE(HAVE_LADSPA, 1, [Has ladspa.h])
	fi
	
	# Check for LV2 (libslv2)
	if test "$build_lv2" = "yes"; then
		build_lv2="no"
		PKG_CHECK_MODULES(SLV2, libslv2 >= 0.0.1, [build_lv2="yes"], [build_lv2="no"])
	fi
	if test "$build_lv2" = "yes"; then
		AC_DEFINE(HAVE_SLV2, 1, [Has libslv2])
	fi
	
	# Check for DSSI
	if test "$build_ladspa" = "no"; then
		AC_MSG_WARN([DSSI support requires LADSPA.  DSSI support will not be built.])
		build_dssi="no"
	fi
	if test "$build_alsa" = "no"; then
		AC_MSG_WARN([DSSI support requires Alsa.  DSSI support will not be built.])
		build_dssi="no"
	fi
	if test "$build_dssi" = "yes"; then
		build_dssi="no"
		AC_CHECK_HEADER([dssi.h], [build_dssi="yes"],
			[AC_MSG_WARN([You do not seem to have dssi.h, Ingen will be built without DSSI support])])
		if test "$build_dssi" = "yes"; then
			AC_DEFINE(HAVE_DSSI, 1, [Has dssi.h])
		fi
	fi
	
else
  AC_MSG_WARN([Engine will NOT be built! (Nothing you're building produces any sound itself)])
fi
AM_CONDITIONAL(BUILD_SERVER, [test "$build_server" = "yes"])
AM_CONDITIONAL(BUILD_IN_PROCESS_ENGINE, [test "$build_in_process_engine" = "yes"])
AM_CONDITIONAL(BUILD_UNIT_TESTS, [test "$build_unit_tests" = "yes"])
AM_CONDITIONAL(WITH_JACK_MIDI, [test "$build_jack_midi" = "yes"])
AM_CONDITIONAL(WITH_ALSA_MIDI, [test "$build_alsa_midi" = "yes"])
AM_CONDITIONAL(WITH_LADSPA, [test "$build_ladspa" = "yes"])
AM_CONDITIONAL(WITH_LV2, [test "$build_lv2" = "yes"])
AM_CONDITIONAL(WITH_DSSI, [test "$build_dssi" = "yes"])



##########################################################################
## Console Clients
##########################################################################

if test "$build_console_clients" = "yes"; then
	
	# Check for libxml2 # FIXME: deprecated, make optional
	PKG_CHECK_MODULES(LXML2, libxml-2.0 >= 2.6.0)
	
	# Check for glibmm
	PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)

	# Check for raptor (for RDF serialization)
	PKG_CHECK_MODULES(RAPTOR, raptor >= 1.4.14, build_raptor="yes", build_raptor="no")
	
	# Check for rasqal (for RDF querying)
	PKG_CHECK_MODULES(RASQAL, rasqal >= 0.9.11, build_rasqal="yes", build_rasqal="no")
	
	# Check for sigc++ (FIXME: make this only necessary where.. uh.. necessary)
	PKG_CHECK_MODULES(LSIGCPP, sigc++-2.0)
else
 	AC_MSG_WARN([Console clients will be built!])
fi
AM_CONDITIONAL(BUILD_CONSOLE_CLIENTS, [test "$build_console_clients" = "yes"])



##########################################################################
## GTK Client (Ingenuity)
##########################################################################

if test "$build_gtk_client" = "yes"; then
  
	# Check misc. system stuff
	AC_CHECK_HEADERS([string.h sys/time.h unistd.h])
	AC_HEADER_TIME
	AC_FUNC_ERROR_AT_LINE
	AC_FUNC_FORK
	AC_FUNC_MALLOC
	AC_FUNC_STAT
	AC_CHECK_FUNCS([gettimeofday mkdir strcasecmp strchr strdup strtol])

	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
	PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
	PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
	PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
	PKG_CHECK_MODULES(CURL, libcurl >= 7.15.0, build_curl="yes", build_curl="no")

else
  AC_MSG_WARN([GTK client will NOT be built!])
fi
AM_CONDITIONAL(BUILD_GTK_CLIENT, [test "$build_gtk_client" = "yes"])

# Build client lib if anything above that depends on it is to be built
AM_CONDITIONAL(BUILD_CLIENT_LIB, [test "$build_console_clients" = "yes" -o "$build_gtk_client" = "yes"])

# Write Makefiles
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/common/Makefile])
AC_CONFIG_FILES([src/common/interface/Makefile])
AC_CONFIG_FILES([src/common/lv2ext/Makefile])
AC_CONFIG_FILES([src/libs/Makefile])
AC_CONFIG_FILES([src/libs/module/Makefile])
AC_CONFIG_FILES([src/libs/engine/Makefile])
AC_CONFIG_FILES([src/libs/engine/tests/Makefile])
AC_CONFIG_FILES([src/libs/engine/events/Makefile])
AC_CONFIG_FILES([src/libs/serialisation/Makefile])
AC_CONFIG_FILES([src/libs/client/Makefile])
AC_CONFIG_FILES([src/libs/gui/Makefile])
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
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Ingen build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building engine:            $build_engine])
AC_MSG_RESULT([   JACK in-process support: $build_in_process_engine])
AC_MSG_RESULT([   Jack MIDI support:       $build_jack_midi])
AC_MSG_RESULT([   ALSA MIDI support:       $build_alsa_midi])
AC_MSG_RESULT([   LV2 Plugin support:      $build_lv2])
AC_MSG_RESULT([   DSSI Plugin support:     $build_dssi])
AC_MSG_RESULT([   LADSPA Plugin support:   $build_ladspa])
AC_MSG_RESULT([   LASH support:            $build_lash])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building console clients:   $build_console_clients])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building GUI:               $build_gtk_client])
AC_MSG_RESULT([])
AC_MSG_RESULT([AGAIN, LASH SUPPORT IS NOT AVAILABLE.])
AC_MSG_RESULT([THERE IS NO LASH SUPPORT IN INGEN AT THIS TIME])
AC_MSG_RESULT([YES IT'S COMING. DO NOT ASK ME ABOUT LASH SUPPORT])
AC_MSG_RESULT([])
AC_MSG_RESULT([Building with flags: $CXXFLAGS])
AC_MSG_RESULT([Please note the above and see README for performance information.])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])
