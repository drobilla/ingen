// Copyright 2021-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Configuration header that defines reasonable defaults at compile time.

  This allows compile-time configuration from the command line, while still
  allowing the source to be built "as-is" without any configuration.  The idea
  is to support an advanced build system with configuration checks, while still
  allowing the code to be simply "thrown at a compiler" with features
  determined from the compiler or system headers.  Everything can be
  overridden, so it should never be necessary to edit this file to build
  successfully.

  To ensure that all configure checks are performed, the build system can
  define INGEN_NO_DEFAULT_CONFIG to disable defaults.  In this case, it must
  define all HAVE_FEATURE symbols below to 1 or 0 to enable or disable
  features.  Any missing definitions will generate a compiler warning.

  To ensure that this header is always included properly, all code that uses
  configuration variables includes this header and checks their value with #if
  (not #ifdef).  Variables like USE_FEATURE are internal and should never be
  defined on the command line.
*/

#ifndef INGEN_CONFIG_H
#define INGEN_CONFIG_H

// Define version unconditionally so a warning will catch a mismatch
#define INGEN_VERSION "0.5.1"

#if !defined(INGEN_NO_DEFAULT_CONFIG)

// We need unistd.h to check _POSIX_VERSION
#	ifndef INGEN_NO_POSIX
#		ifdef __has_include
#			if __has_include(<unistd.h>)
#				include <unistd.h>
#			endif
#		elif defined(__unix__)
#			include <unistd.h>
#		endif
#	endif

// POSIX.1-2001: fileno()
#	ifndef HAVE_FILENO
#		if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#			define HAVE_FILENO 1
#		else
#			define HAVE_FILENO 0
#		endif
#	endif

// POSIX.1-2001: isatty()
#	ifndef HAVE_ISATTY
#		if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#			define HAVE_ISATTY 1
#		else
#			define HAVE_ISATTY 0
#		endif
#	endif

// POSIX.1-2001: posix_memalign()
#	ifndef HAVE_POSIX_MEMALIGN
#		if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#			define HAVE_POSIX_MEMALIGN 1
#		else
#			define HAVE_POSIX_MEMALIGN 0
#		endif
#	endif

// BSD and GNU: vasprintf()
#	ifndef HAVE_VASPRINTF
#		if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
#			define HAVE_VASPRINTF 1
#		else
#			define HAVE_VASPRINTF 0
#		endif
#	endif

// JACK
#	ifndef HAVE_JACK
#		ifdef __has_include
#			if __has_include("jack/jack.h")
#				define HAVE_JACK 1
#			else
#				define HAVE_JACK 0
#			endif
#		else
#			define HAVE_JACK 0
#		endif
#	endif

// JACK metadata API
#	ifndef HAVE_JACK_METADATA
#		ifdef __has_include
#			if __has_include("jack/metadata.h")
#				define HAVE_JACK_METADATA 1
#			else
#				define HAVE_JACK_METADATA 0
#			endif
#		else
#			define HAVE_JACK_METADATA 0
#		endif
#	endif

// JACK jack_port_rename() function
#	ifndef HAVE_JACK_PORT_RENAME
#		define HAVE_JACK_PORT_RENAME HAVE_JACK
#	endif

// BSD sockets
#	ifndef HAVE_SOCKET
#		ifdef __has_include
#			if __has_include("sys/socket.h")
#				define HAVE_SOCKET 1
#			else
#				define HAVE_SOCKET 0
#			endif
#		else
#			define HAVE_SOCKET 0
#		endif
#	endif

// Webkit
#	ifndef HAVE_WEBKIT
#		ifdef __has_include
#			if __has_include(<webkit/webkit.h>)
#				define HAVE_WEBKIT 1
#			else
#				define HAVE_WEBKIT 0
#			endif
#		else
#			define HAVE_WEBKIT 0
#		endif
#	endif

// Installation directories
#	ifndef INGEN_DATA_DIR
#		define INGEN_DATA_DIR "/usr/local/share/ingen"
#	endif
#	ifndef INGEN_MODULE_DIR
#		define INGEN_MODULE_DIR "/usr/local/lib/ingen"
#	endif
#	ifndef INGEN_BUNDLE_DIR
#		define INGEN_BUNDLE_DIR "/usr/local/lib/lv2/ingen.lv2"
#	endif

#endif // !defined(INGEN_NO_DEFAULT_CONFIG)

/*
  Make corresponding USE_FEATURE defines based on the HAVE_FEATURE defines from
  above or the command line.  The code checks for these using #if (not #ifdef),
  so there will be an undefined warning if it checks for an unknown feature,
  and this header is always required by any code that checks for features, even
  if the build system defines them all.
*/

#if defined(HAVE_FILENO)
#	define USE_FILENO HAVE_FILENO
#else
#	define USE_FILENO 0
#endif

#if defined(HAVE_ISATTY)
#	define USE_ISATTY HAVE_ISATTY
#else
#	define USE_ISATTY 0
#endif

#if defined(HAVE_POSIX_MEMALIGN)
#	define USE_POSIX_MEMALIGN HAVE_POSIX_MEMALIGN
#else
#	define USE_POSIX_MEMALIGN 0
#endif

#if defined(HAVE_SOCKET)
#	define USE_SOCKET HAVE_SOCKET
#else
#	define USE_SOCKET 0
#endif

#if defined(HAVE_VASPRINTF)
#	define USE_VASPRINTF HAVE_VASPRINTF
#else
#	define USE_VASPRINTF 0
#endif

#if defined(HAVE_WEBKIT)
#	define USE_WEBKIT HAVE_WEBKIT
#else
#	define USE_WEBKIT 0
#endif

#if defined(HAVE_JACK_METADATA)
#	define USE_JACK_METADATA HAVE_JACK_METADATA
#else
#	define USE_JACK_METADATA 0
#endif

#if defined(HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE)
#	define USE_JACK_PORT_TYPE_GET_BUFFER_SIZE \
		HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE
#else
#	define USE_JACK_PORT_TYPE_GET_BUFFER_SIZE 0
#endif

#if defined(HAVE_JACK_PORT_RENAME)
#	define USE_JACK_PORT_RENAME HAVE_JACK_PORT_RENAME
#else
#	define USE_JACK_PORT_RENAME 0
#endif

#define INGEN_BUNDLED 0

#endif // INGEN_CONFIG_H
