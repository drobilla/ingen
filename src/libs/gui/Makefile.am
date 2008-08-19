MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST = ingen_gui.gladep

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = ingen.svg ingen-icon.svg

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = ingen_gui.glade ingen.svg ingen-icon.svg

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_gui.la

libingen_gui_la_CXXFLAGS = \
	-DINGEN_MODULE_DIR=\"$(libdir)/ingen\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	@CURL_CFLAGS@ \
	@FLOWCANVAS_CFLAGS@ \
	@GNOMECANVASMM_CFLAGS@ \
	@GTKMM_CFLAGS@ \
	@INGEN_CFLAGS@ \
	@LIBGLADEMM_CFLAGS@ \
	@LIBLO_CFLAGS@ \
	@REDLANDMM_CFLAGS@ \
	@RAUL_CFLAGS@ \
	@SLV2_CFLAGS@ \
	@SOUP_CFLAGS@
	

libingen_gui_la_LDFLAGS = -no-undefined -module -avoid-version

libingen_gui_la_LIBADD = \
	../module/libingen_module.la \
	../shared/libingen_shared.la \
	../client/libingen_client.la \
	@CURL_LIBS@ \
	@FLOWCANVAS_LIBS@ \
	@GNOMECANVASMM_LIBS@ \
	@GTKMM_LIBS@ \
	@LIBGLADEMM_LIBS@ \
	@LIBLO_LIBS@ \
	@REDLANDMM_LIBS@ \
	@RAUL_LIBS@ \
	@SLV2_LIBS@ \
	@SOUP_LIBS@

libingen_gui_la_SOURCES = \
	gui.hpp \
	gui.cpp \
	App.cpp \
	App.hpp \
	BreadCrumb.hpp \
	BreadCrumbBox.cpp \
	BreadCrumbBox.hpp \
	Configuration.cpp \
	Configuration.hpp \
	ConnectWindow.cpp \
	ConnectWindow.hpp \
	Connection.hpp \
	ControlPanel.cpp \
	ControlPanel.hpp \
	Controls.cpp \
	Controls.hpp \
	GladeFactory.cpp \
	GladeFactory.hpp \
	LoadPatchWindow.cpp \
	LoadPatchWindow.hpp \
	LoadPluginWindow.cpp \
	LoadPluginWindow.hpp \
	LoadRemotePatchWindow.cpp \
	LoadRemotePatchWindow.hpp \
	LoadSubpatchWindow.cpp \
	LoadSubpatchWindow.hpp \
	MessagesWindow.cpp \
	MessagesWindow.hpp \
	NewSubpatchWindow.cpp \
	NewSubpatchWindow.hpp \
	NodeControlWindow.cpp \
	NodeControlWindow.hpp \
	ObjectMenu.hpp \
	ObjectMenu.cpp \
	NodeMenu.hpp \
	NodeMenu.cpp \
	PortMenu.hpp \
	PortMenu.cpp \
	NodeModule.cpp \
	NodeModule.hpp \
	NodePropertiesWindow.cpp \
	NodePropertiesWindow.hpp \
	PatchCanvas.cpp \
	PatchCanvas.hpp \
	PatchPortModule.cpp \
	PatchPortModule.hpp \
	PatchPropertiesWindow.cpp \
	PatchPropertiesWindow.hpp \
	PatchTreeWindow.cpp \
	PatchTreeWindow.hpp \
	PatchView.cpp \
	PatchView.hpp \
	PatchWindow.cpp \
	PatchWindow.hpp \
	Port.cpp \
	Port.hpp \
	PortPropertiesWindow.cpp \
	PortPropertiesWindow.hpp \
	RenameWindow.cpp \
	RenameWindow.hpp \
	SubpatchModule.cpp \
	SubpatchModule.hpp \
	ThreadedLoader.cpp \
	ThreadedLoader.hpp \
	WindowFactory.cpp \
	WindowFactory.hpp
	
if WITH_CURL
libingen_gui_la_SOURCES += \
	UploadPatchWindow.cpp \
	UploadPatchWindow.hpp
endif



