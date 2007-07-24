MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST = ingen_gui.gladep

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = ingen.svg ingen-icon.svg

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = ingen_gui.glade ingen.svg ingen-icon.svg

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_gui.la

libingen_gui_la_CXXFLAGS = \
	-I$(top_srcdir)/slv2 \
	-I$(top_srcdir)/raul \
	-I$(top_srcdir)/flowcanvas \
	-I$(top_srcdir)/ingen/src/common \
	-I$(top_srcdir)/ingen/src/libs \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DINGEN_MODULE_DIR=\"$(libdir)/ingen\" \
	-DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
	@LIBLO_CFLAGS@ @CURL_CFLAGS@ \
	@GTKMM_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ 

libingen_gui_la_LDFLAGS = -no-undefined -module -avoid-version

libingen_gui_la_LIBADD = \
	@RAUL_LIBS@ @LIBLO_LIBS@ @SLV2_LIBS@ @CURL_LIBS@ \
	@GTKMM_LIBS@ @LIBGLADEMM_LIBS@ @FLOWCANVAS_LIBS@ @GNOMECANVASMM_LIBS@ \
	../module/libingen_module.la \
	../client/libingen_client.la


libingen_gui_la_SOURCES = \
	gui.h \
	gui.cpp \
	App.cpp \
	App.h \
	BreadCrumb.h \
	BreadCrumbBox.cpp \
	BreadCrumbBox.h \
	ConfigWindow.cpp \
	ConfigWindow.h \
	Configuration.cpp \
	Configuration.h \
	ConnectWindow.cpp \
	ConnectWindow.h \
	Connection.h \
	ControlGroups.cpp \
	ControlGroups.h \
	ControlPanel.cpp \
	ControlPanel.h \
	DSSIController.cpp \
	DSSIController.h \
	DSSIModule.cpp \
	DSSIModule.h \
	GladeFactory.cpp \
	GladeFactory.h \
	LoadPatchWindow.cpp \
	LoadPatchWindow.h \
	LoadPluginWindow.cpp \
	LoadPluginWindow.h \
	LoadRemotePatchWindow.cpp \
	LoadRemotePatchWindow.h \
	LoadSubpatchWindow.cpp \
	LoadSubpatchWindow.h \
	MessagesWindow.cpp \
	MessagesWindow.h \
	NewSubpatchWindow.cpp \
	NewSubpatchWindow.h \
	NodeControlWindow.cpp \
	NodeControlWindow.h \
	NodeMenu.cpp \
	NodeMenu.h \
	NodeModule.cpp \
	NodeModule.h \
	NodePropertiesWindow.cpp \
	NodePropertiesWindow.h \
	PatchCanvas.cpp \
	PatchCanvas.h \
	PatchPortModule.cpp \
	PatchPortModule.h \
	PatchPropertiesWindow.cpp \
	PatchPropertiesWindow.h \
	PatchTreeWindow.cpp \
	PatchTreeWindow.h \
	PatchView.cpp \
	PatchView.h \
	PatchWindow.cpp \
	PatchWindow.h \
	Port.cpp \
	Port.h \
	PortPropertiesWindow.cpp \
	PortPropertiesWindow.h \
	RenameWindow.cpp \
	RenameWindow.h \
	SubpatchModule.cpp \
	SubpatchModule.h \
	ThreadedLoader.cpp \
	ThreadedLoader.h \
	UploadPatchWindow.cpp \
	UploadPatchWindow.h \
	WindowFactory.cpp \
	WindowFactory.h


