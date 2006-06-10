AM_CXXFLAGS = -I$(top_srcdir)/src/common -fno-exceptions -fno-rtti

SUBDIRS = . python supercollider

if BUILD_CONSOLE_CLIENTS
SUBDIRS += patch_loader patches demolition
endif

if BUILD_GTK_CLIENT
SUBDIRS += gtk
endif

