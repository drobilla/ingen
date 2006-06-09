if BUILD_GTK_CLIENT

EXTRA_DIST = om_gtk.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = om_gtk.glade om-icon.png

AM_CXXFLAGS = -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -I$(top_srcdir)/src/common -I$(top_srcdir)/src/clients -DPKGDATADIR=\"$(pkgdatadir)\" @GTKMM_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @LOSC_CFLAGS@ @LASH_CFLAGS@ @FLOWCANVAS_CFLAGS@
om_gtk_LDADD = @GTKMM_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @LOSC_LIBS@ @LASH_LIBS@ @FLOWCANVAS_LIBS@ ../libomclient.a
om_gtk_DEPENDENCIES = ../libomclient.a


bin_PROGRAMS = om_gtk
om_gtk_SOURCES = \
	cmdline.h \
	cmdline.c \
	main.cpp \
	singletons.cpp \
	ConnectWindow.h \
	ConnectWindow.cpp \
	App.h \
	App.cpp \
	Store.h \
	Store.cpp \
	Configuration.h \
	Configuration.cpp \
	GladeFactory.h \
	GladeFactory.cpp \
	Controller.h \
	Controller.cpp \
	GtkObjectController.h \
	GtkObjectController.cpp \
	PatchController.h \
	PatchController.cpp \
	NodeController.h \
	NodeController.cpp \
	PortController.h \
	PortController.cpp \
	DSSIController.h \
	DSSIController.cpp \
	LoadPluginWindow.h \
	LoadPluginWindow.cpp \
	LoadPatchWindow.h \
	LoadPatchWindow.cpp \
	MessagesWindow.h \
	MessagesWindow.cpp \
	LoadSubpatchWindow.h \
	LoadSubpatchWindow.cpp \
	ControlInterface.h \
	ControlInterface.cpp \
	NodeControlWindow.h \
	NodeControlWindow.cpp \
	ControlPanel.h \
	ControlPanel.cpp \
	ControlGroups.h \
	ControlGroups.cpp \
	PatchView.h \
	PatchView.cpp \
	PatchWindow.h \
	PatchWindow.cpp \
	BreadCrumb.h \
	OmFlowCanvas.h \
	OmFlowCanvas.cpp \
	../../common/types.h \
	../../common/Path.h \
	OmModule.h \
	OmModule.cpp \
	DSSIModule.h \
	DSSIModule.cpp \
	SubpatchModule.h \
	SubpatchModule.cpp \
	OmPort.h \
	OmPort.cpp \
	NewSubpatchWindow.h \
	NewSubpatchWindow.cpp \
	ConfigWindow.h \
	ConfigWindow.cpp \
	PatchDescriptionWindow.h \
	PatchDescriptionWindow.cpp \
	Loader.h \
	Loader.cpp \
	RenameWindow.h \
	RenameWindow.cpp \
	NodePropertiesWindow.h \
	NodePropertiesWindow.cpp \
	PatchTreeWindow.h \
	PatchTreeWindow.cpp

#GtkClientInterface.h
#GtkClientInterface.cpp


if WITH_LASH
om_gtk_SOURCES += LashController.h LashController.cpp
endif


endif # WITH_GTK_CLIENT
