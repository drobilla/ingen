SUBDIRS = tests events

MAINTAINERCLEANFILES = Makefile.in

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_engine.la
libingen_engine_la_CXXFLAGS = \
	-I$(top_srcdir)/slv2 \
	-I$(top_srcdir)/raul \
	-I$(top_srcdir)/ingen/src/common \
	-I$(top_srcdir)/ingen/src/libs \
	-I$(top_srcdir)/ingen/src/libs/engine/events \
	@JACK_CFLAGS@ @LIBLO_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @GLIBMM_CFLAGS@

libingen_engine_la_LDFLAGS = -no-undefined -module -avoid-version
libingen_engine_la_LIBADD = @RAUL_LIBS@ @JACK_LIBS@ @LIBLO_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @SLV2_LIBS@ @GLIBMM_LIBS@

AM_CFLAGS=-std=c99

libingen_engine_la_SOURCES = \
	../../../../lv2/extensions/osc/lv2_osc.c \
	../../../../lv2/extensions/osc/lv2_osc_print.c \
	AudioBuffer.cpp \
	AudioBuffer.hpp \
	AudioDriver.hpp \
	Buffer.hpp \
	BufferFactory.cpp \
	BufferFactory.hpp \
	ClientBroadcaster.cpp \
	ClientBroadcaster.hpp \
	Connection.cpp \
	Connection.hpp \
	DataType.hpp \
	DirectResponder.hpp \
	Driver.hpp \
	DuplexPort.cpp \
	DuplexPort.hpp \
	Engine.cpp \
	Engine.hpp \
	Event.hpp \
	EventSource.hpp \
	GraphObject.cpp \
	GraphObject.hpp \
	InputPort.cpp \
	InputPort.hpp \
	InternalNode.hpp \
	JackAudioDriver.cpp \
	JackAudioDriver.hpp \
	JackMidiDriver.cpp \
	JackMidiDriver.hpp \
	MidiBuffer.cpp \
	MidiBuffer.hpp \
	MidiControlNode.cpp \
	MidiControlNode.hpp \
	MidiDriver.hpp \
	MidiNoteNode.cpp \
	MidiNoteNode.hpp \
	MidiTriggerNode.cpp \
	MidiTriggerNode.hpp \
	Node.hpp \
	NodeBase.cpp \
	NodeBase.hpp \
	NodeFactory.cpp \
	NodeFactory.hpp \
	OSCBuffer.cpp \
	OSCBuffer.hpp \
	OSCClientSender.cpp \
	OSCClientSender.hpp \
	OSCDriver.hpp \
	OSCEngineReceiver.cpp \
	OSCEngineReceiver.hpp \
	OSCResponder.cpp \
	OSCResponder.hpp \
	ObjectSender.cpp \
	ObjectSender.hpp \
	ObjectStore.cpp \
	ObjectStore.hpp \
	OutputPort.cpp \
	OutputPort.hpp \
	Patch.cpp \
	Patch.hpp \
	Plugin.cpp \
	Plugin.hpp \
	Port.cpp \
	Port.hpp \
	PostProcessor.cpp \
	PostProcessor.hpp \
	QueuedEngineInterface.cpp \
	QueuedEngineInterface.hpp \
	QueuedEvent.hpp \
	QueuedEventSource.cpp \
	QueuedEventSource.hpp \
	ThreadManager.hpp \
	TransportNode.cpp \
	TransportNode.hpp \
	Tree.hpp \
	TreeImplementation.hpp \
	engine.cpp \
	engine.hpp \
	events.hpp \
	events/AddNodeEvent.cpp \
	events/AddNodeEvent.hpp \
	events/AddPortEvent.cpp \
	events/AddPortEvent.hpp \
	events/AllNotesOffEvent.cpp \
	events/AllNotesOffEvent.hpp \
	events/ClearPatchEvent.cpp \
	events/ClearPatchEvent.hpp \
	events/ConnectionEvent.cpp \
	events/ConnectionEvent.hpp \
	events/CreatePatchEvent.cpp \
	events/CreatePatchEvent.hpp \
	events/DeactivateEvent.cpp \
	events/DeactivateEvent.hpp \
	events/DestroyEvent.cpp \
	events/DestroyEvent.hpp \
	events/DisablePatchEvent.cpp \
	events/DisablePatchEvent.hpp \
	events/DisconnectNodeEvent.cpp \
	events/DisconnectNodeEvent.hpp \
	events/DisconnectPortEvent.cpp \
	events/DisconnectPortEvent.hpp \
	events/DisconnectionEvent.cpp \
	events/DisconnectionEvent.hpp \
	events/EnablePatchEvent.cpp \
	events/EnablePatchEvent.hpp \
	events/LoadPluginsEvent.cpp \
	events/LoadPluginsEvent.hpp \
	events/MidiLearnEvent.cpp \
	events/MidiLearnEvent.hpp \
	events/NoteOffEvent.cpp \
	events/NoteOffEvent.hpp \
	events/NoteOnEvent.cpp \
	events/NoteOnEvent.hpp \
	events/PingQueuedEvent.hpp \
	events/RegisterClientEvent.cpp \
	events/RegisterClientEvent.hpp \
	events/RenameEvent.cpp \
	events/RenameEvent.hpp \
	events/RequestAllObjectsEvent.cpp \
	events/RequestAllObjectsEvent.hpp \
	events/RequestMetadataEvent.cpp \
	events/RequestMetadataEvent.hpp \
	events/RequestObjectEvent.cpp \
	events/RequestObjectEvent.hpp \
	events/RequestPluginEvent.cpp \
	events/RequestPluginEvent.hpp \
	events/RequestPluginsEvent.cpp \
	events/RequestPluginsEvent.hpp \
	events/RequestPortValueEvent.cpp \
	events/RequestPortValueEvent.hpp \
	events/SetMetadataEvent.cpp \
	events/SetMetadataEvent.hpp \
	events/SetPortValueEvent.cpp \
	events/SetPortValueEvent.hpp \
	events/SetPortValueQueuedEvent.cpp \
	events/SetPortValueQueuedEvent.hpp \
	events/UnregisterClientEvent.cpp \
	events/UnregisterClientEvent.hpp \
	instantiations.cpp \
	jack_compat.hpp \
	tuning.hpp \
	types.hpp \
	util.hpp

if WITH_LADSPA
libingen_engine_la_SOURCES += \
	LADSPANode.hpp \
	LADSPANode.cpp
endif

if WITH_DSSI
libingen_engine_la_SOURCES += \
	DSSINode.hpp \
	DSSINode.cpp \
	events/DSSIConfigureEvent.cpp \
	events/DSSIConfigureEvent.hpp \
	events/DSSIControlEvent.cpp \
	events/DSSIControlEvent.hpp \
	events/DSSIProgramEvent.cpp \
	events/DSSIProgramEvent.hpp \
	events/DSSIUpdateEvent.cpp \
	events/DSSIUpdateEvent.hpp
endif

if WITH_LV2
libingen_engine_la_SOURCES += \
	LV2Node.hpp \
	LV2Node.cpp
endif

#if WITH_LASH
#libingen_engine_la_SOURCES += \
#	LashDriver.hpp \
#	LashDriver.cpp
#endif

