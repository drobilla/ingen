AC_PREREQ(2.59)
AC_INIT([om],[0.3.0pre],[drobilla@connect.carleton.ca])
AC_CONFIG_SRCDIR([src/common/util/CountedPtr.h])
AC_CONFIG_SRCDIR([src/common/interface/EngineInterface.h])
AC_CONFIG_SRCDIR([src/libs/engine/JackAudioDriver.cpp])
AC_CONFIG_SRCDIR([src/libs/engine/tests/node_tree_test.cpp])
AC_CONFIG_SRCDIR([src/libs/client/OSCController.cpp])
AC_CONFIG_SRCDIR([src/progs/engine/main.cpp])
AC_CONFIG_SRCDIR([src/progs/patch_loader/patch_loader.cpp])
AC_CONFIG_SRCDIR([src/progs/demolition/demolition.cpp])
AC_CONFIG_SRCDIR([src/progs/gtk/main.cpp])
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
AC_SUBST(LOSC_LIBS)
AC_SUBST(LOSC_CFLAGS)

# Check for pthreads
AC_CHECK_LIB(pthread, pthread_create, [],
	AC_MSG_ERROR([*** Om requires POSIX threads support]))
AC_CHECK_HEADER(pthread.h)

# Check for POSIX semaphores in rt library
AC_CHECK_HEADERS([semaphore.h])
AC_CHECK_LIB([rt], [sem_post])

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
	AC_SUBST(LASH_CFLAGS)
	AC_SUBST(LASH_LIBS)
else
	AC_MSG_WARN([LASH not found, session support will not be built.])
fi
AM_CONDITIONAL(WITH_LASH, [test "$build_lash" = "yes"])

# Use debugging flag?
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (no)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  CFLAGS="-O0 -g -DDEBUG"
  CXXFLAGS="$CFLAGS"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CFLAGS -DNDEBUG"
fi

# Use strict flags?
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (no)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  # Stupid Gtkmm won't build with -pedantic
  CFLAGS="$CFLAGS -ansi -Wall -Wconversion -Winit-self"
  CXXFLAGS="$CXXFLAGS -ansi -Wall -Wconversion -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

# Bolt on a few specific flags to CFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -fmessage-length=999"
CFLAGS="$CFLAGS -pipe -fmessage-length=999"



##########################################################################
## ENGINE
##########################################################################

# Check for building engine
build_engine="yes"
AC_ARG_ENABLE([engine],
	AS_HELP_STRING(--enable-engine, Build engine (yes)),
	[ if test x$enable_engine = xno ; then build_engine=no ; fi ])

# Check for building engine as an in-process jack client
build_in_process_engine="no"
AC_ARG_ENABLE([in-process-engine],
	AS_HELP_STRING(--enable-in-process-engine, Build engine as an in-process Jack client (no)),
	[ if test x$enable_in_process_engine = xyes ; then build_in_process_engine=yes ; fi ])

# Jack MIDI option
build_jack_midi="no"
AC_ARG_ENABLE(jack-midi,
	[AS_HELP_STRING(--enable-jack-midi, [Enable Jack MIDI support (no)])],
	[ if test x$enable_jack_midi = xyes ; then build_jack_midi=yes ; fi ])

# ALSA option
build_alsa_midi="yes"
AC_ARG_ENABLE(alsa-midi,
	[AS_HELP_STRING(--enable-alsa-midi, [Enable Alsa MIDI driver (yes)])],
	[ if test x$enable_alsa_midi = xno ; then build_alsa_midi=no ; fi ])

# LV2 option
build_lv2="yes"
AC_ARG_ENABLE(lv2,
	[AS_HELP_STRING(--enable-lv2, [Enable LV2 plugin support through libslv2 (yes)])],
	[ if test x$enable_lv2 = xno ; then build_lv2=no ; fi ])

# DSSI option
build_dssi="yes"
AC_ARG_ENABLE(dssi,
	[AS_HELP_STRING(--enable-dssi, [Enable DSSI plugin support (yes, requires Alsa)])],
	[ if test x$enable_dssi = xno ; then build_dssi=no ; fi ])

# LADSPA option
build_ladspa="yes"
AC_ARG_ENABLE(ladspa,
	[AS_HELP_STRING(--enable-ladspa, [Enable LADSPA plugin support (yes)])],
	[ if test x$enable_ladspa = xno ; then build_ladspa=no ; fi ])


if test "$build_in_process_engine" = "yes"; then
	AC_DEFINE(BUILD_IN_PROCESS_ENGINE, 1, [Whether to build engine as Jack in-process client])
fi


if test "$build_engine" = "yes" -o "$build_in_process_engine" = "yes"; then
	# Check for standard lib stuff
	AC_CHECK_FUNCS([posix_memalign, fesetround])
	AC_CHECK_HEADERS([float.h stddef.h fenv.h])
	
	# Check random other stuff
	AC_FUNC_CLOSEDIR_VOID
	
	# Check for Jack
	PKG_CHECK_MODULES(JACK, jack >= 0.99.0)
	AC_SUBST(JACK_LIBS)
	AC_SUBST(JACK_CFLAGS)

	# Check for Jack MIDI
	if test "$build_jack_midi" = "yes"; then
		AC_CHECK_HEADER([jack/midiport.h], [build_jack_midi="yes"],
			[AC_MSG_WARN([You don't seem to build jack/midiport.h, Jack MIDI support will not be built])])
	fi
	if test "$build_jack_midi" = "yes"; then
		AC_DEFINE(HAVE_JACK_MIDI, 1, [Has Jack MIDI])
	fi
	
	# Check for ALSA
	if test "$build_alsa_midi" = "yes" -o "$build_dssi" = "yes"; then
		PKG_CHECK_MODULES(ALSA, alsa >= 1.0.0, [have_alsa_midi="yes"], [have_alsa_midi="no"])
		AC_SUBST(ALSA_CFLAGS)
		AC_SUBST(ALSA_LIBS)
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
		AC_CHECK_HEADER(ladspa.h, [build_ladspa="yes"],
			[AC_MSG_WARN([You don't seem to build ladspa.h, Om will not be very useful!])])
	fi
	if test "$build_ladspa" = "yes"; then
		AC_DEFINE(HAVE_LADSPA, 1, [Has ladspa.h])
	fi
	
	# Check for LV2 (libslv2)
	if test "$build_lv2" = "yes"; then
		build_lv2="no"
		PKG_CHECK_MODULES(SLV2, libslv2 >= 0.0.1, [build_lv2="yes"], [build_lv2="no"])
		AC_SUBST(SLV2_CFLAGS)
		AC_SUBST(SLV2_LIBS)
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
		AC_CHECK_HEADER(dssi.h, [build_dssi="yes"],
			[AC_MSG_WARN([You do not seem to have dssi.h, Om will be built without DSSI support])])
		if test "$build_dssi" = "yes"; then
			AC_DEFINE(HAVE_DSSI, 1, [Has dssi.h])
		fi
	fi
	
	# Build unit tests?
	build_unit_tests="no"
	AC_ARG_ENABLE(unit-tests,
		[AS_HELP_STRING(--enable-unit-tests, [Build unit tests (no)])],
		[build_unit_tests="$enableval"])
	
else
  AC_MSG_WARN([Engine will NOT be built! (Nothing you're building produces sound)])
fi
AM_CONDITIONAL(BUILD_ENGINE, [test "$build_engine" = "yes"])
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

build_console_clients="yes"
AC_ARG_ENABLE([console-clients],
	AS_HELP_STRING(--enable-console-clients, Build console clients (true, requires libxml2)),
	[ if test x$enable_console_clients = xno ; then build_console_clients=no ; fi ])

if test "$build_console_clients" = "yes"; then

	# Check for libxml2
	PKG_CHECK_MODULES(LXML2, libxml-2.0 >= 2.6.0)
	AC_SUBST(LXML2_LIBS)
	AC_SUBST(LXML2_CFLAGS)
	
	# Check for sigc++ (FIXME: make this only necessary where.. uh.. necessary)
	PKG_CHECK_MODULES(LSIGCPP, sigc++-2.0)
	AC_SUBST(LSIGCPP_CFLAGS)
	AC_SUBST(LSIGCPP_LIBS)
else
 	AC_MSG_WARN([Console clients will be built!])
fi
AM_CONDITIONAL(BUILD_CONSOLE_CLIENTS, [test "$build_console_clients" = "yes"])



##########################################################################
## GTK Client
##########################################################################

# Check for building gtk client
build_gtk_client="yes"
AC_ARG_ENABLE([gtk-client],
	AS_HELP_STRING(--enable-gtk-client, Build GTK client. (true, requires console clients)),
	[ if test x$enable_gtk_client = xno ; then build_gtk_client=no ; fi ])

if test "$build_console_clients" = "no"; then
	AC_MSG_WARN([GTK client will NOT be built! (requires console clients)])
	build_gtk_client=no
fi
if test "$build_gtk_client" = "yes"; then
  
	# Checks for header files.
	AC_CHECK_HEADERS([string.h sys/time.h unistd.h])
	
	# Checks for typedefs, structures, and compiler characteristics.
	AC_HEADER_TIME
	
	# Checks for library functions.
	AC_FUNC_ERROR_AT_LINE
	AC_FUNC_FORK
	AC_FUNC_MALLOC
	AC_FUNC_STAT
	AC_CHECK_FUNCS([gettimeofday mkdir strcasecmp strchr strdup strtol])

	# Check for GTKMM
	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
	AC_SUBST(GTKMM_CFLAGS)
	AC_SUBST(GTKMM_LIBS)
	
	# Check for gnomecanvasmm
	PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
	AC_SUBST(GNOMECANVASMM_CFLAGS)
	AC_SUBST(GNOMECANVASMM_LIBS)
	
	# Check for libglademm
	PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
	AC_SUBST(LIBGLADEMM_CFLAGS)
	AC_SUBST(LIBGLADEMM_LIBS)
	
	# Check for FlowCanvas
	PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
	AC_SUBST(FLOWCANVAS_CFLAGS)
	AC_SUBST(FLOWCANVAS_LIBS)
	
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
AC_CONFIG_FILES([src/common/util/Makefile])
AC_CONFIG_FILES([src/common/interface/Makefile])
AC_CONFIG_FILES([src/libs/Makefile])
AC_CONFIG_FILES([src/libs/engine/Makefile])
AC_CONFIG_FILES([src/libs/engine/tests/Makefile])
AC_CONFIG_FILES([src/libs/client/Makefile])
AC_CONFIG_FILES([src/progs/Makefile])
AC_CONFIG_FILES([src/progs/om/Makefile])
AC_CONFIG_FILES([src/progs/supercollider/Makefile])
AC_CONFIG_FILES([src/progs/python/Makefile])
AC_CONFIG_FILES([src/progs/python/scripts/Makefile])
AC_CONFIG_FILES([src/progs/demolition/Makefile])
AC_CONFIG_FILES([src/progs/patch_loader/Makefile])
AC_CONFIG_FILES([src/progs/gtk/Makefile])
AC_CONFIG_FILES([patches/Makefile])
AC_CONFIG_FILES([doc/Doxyfile])
AC_OUTPUT

# Display summary
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Om build configuration:])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Building engine:            $build_engine])
AC_MSG_NOTICE([   Jack MIDI support:       $build_jack_midi])
AC_MSG_NOTICE([   ALSA MIDI support:       $build_alsa_midi])
AC_MSG_NOTICE([   LV2 Plugin support:      $build_lv2])
AC_MSG_NOTICE([   DSSI Plugin support:     $build_dssi])
AC_MSG_NOTICE([   LADSPA Plugin support:   $build_ladspa])
AC_MSG_NOTICE([   LASH support:            $build_lash])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Building in-process engine: $build_in_process_engine])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Building console clients:   $build_console_clients])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Building gtk client:        $build_gtk_client])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Building with flags: $CXXFLAGS])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Please note the above and see README for performance information.])
