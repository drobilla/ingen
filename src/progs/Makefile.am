AM_CXXFLAGS = -I$(top_srcdir)/src/common -fno-exceptions -fno-rtti

DIST_SUBDIRS = python supercollider

SUBDIRS = om

if BUILD_CONSOLE_CLIENTS
SUBDIRS += patch_loader demolition
endif

if BUILD_GTK_CLIENT
SUBDIRS += gtk
endif


