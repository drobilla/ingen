SUBDIRS = events

MAINTAINERCLEANFILES = Makefile.in

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_engine.la
libingen_engine_la_CXXFLAGS = \
	@SLV2_CFLAGS@ @RAUL_CFLAGS@ \
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
	ConnectionImpl.cpp \
	ConnectionImpl.hpp \
	CompiledPatch.hpp \
	Driver.hpp \
	DuplexPort.cpp \
	DuplexPort.hpp \
	Engine.cpp \
	Engine.hpp \
	Event.cpp \
	Event.hpp \
	EventSink.cpp \
	EventSink.hpp \
	EventSource.hpp \
	GraphObjectImpl.cpp \
	GraphObjectImpl.hpp \
	InputPort.cpp \
	InputPort.hpp \
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
	Responder.hpp \
	ObjectSender.cpp \
	ObjectSender.hpp \
	ObjectStore.cpp \
	ObjectStore.hpp \
	OutputPort.cpp \
	OutputPort.hpp \
	PatchImpl.cpp \
	PatchImpl.hpp \
	PluginImpl.cpp \
	PluginImpl.hpp \
	PortImpl.cpp \
	PortImpl.hpp \
	PostProcessor.cpp \
	PostProcessor.hpp \
	ProcessContext.hpp \
	ProcessContext.cpp \
	ProcessSlave.hpp \
	ProcessSlave.cpp \
	QueuedEngineInterface.cpp \
	QueuedEngineInterface.hpp \
	QueuedEvent.cpp \
	QueuedEvent.hpp \
	QueuedEventSource.cpp \
	QueuedEventSource.hpp \
	ThreadManager.hpp \
	TransportNode.cpp \
	TransportNode.hpp \
	engine.cpp \
	engine.hpp \
	events.hpp \
	events/AllNotesOffEvent.cpp \
	events/AllNotesOffEvent.hpp \
	events/ClearPatchEvent.cpp \
	events/ClearPatchEvent.hpp \
	events/ConnectionEvent.cpp \
	events/ConnectionEvent.hpp \
	events/CreateNodeEvent.cpp \
	events/CreateNodeEvent.hpp \
	events/CreatePatchEvent.cpp \
	events/CreatePatchEvent.hpp \
	events/CreatePortEvent.cpp \
	events/CreatePortEvent.hpp \
	events/DeactivateEvent.cpp \
	events/DeactivateEvent.hpp \
	events/DestroyEvent.cpp \
	events/DestroyEvent.hpp \
	events/DisconnectNodeEvent.cpp \
	events/DisconnectNodeEvent.hpp \
	events/DisconnectPortEvent.cpp \
	events/DisconnectPortEvent.hpp \
	events/DisconnectionEvent.cpp \
	events/DisconnectionEvent.hpp \
	events/EnablePatchEvent.cpp \
	events/EnablePatchEvent.hpp \
	events/EnablePortBroadcastingEvent.cpp \
	events/EnablePortBroadcastingEvent.hpp \
	events/LoadPluginsEvent.cpp \
	events/LoadPluginsEvent.hpp \
	events/MidiLearnEvent.cpp \
	events/MidiLearnEvent.hpp \
	events/NoteEvent.cpp \
	events/NoteEvent.hpp \
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
	events/SendPortActivityEvent.cpp \
	events/SendPortActivityEvent.hpp \
	events/SendPortValueEvent.cpp \
	events/SendPortValueEvent.hpp \
	events/SetMetadataEvent.cpp \
	events/SetMetadataEvent.hpp \
	events/SetPolyphonicEvent.cpp \
	events/SetPolyphonicEvent.hpp \
	events/SetPolyphonyEvent.cpp \
	events/SetPolyphonyEvent.hpp \
	events/SetPortValueEvent.cpp \
	events/SetPortValueEvent.hpp \
	events/SetPortValueQueuedEvent.cpp \
	events/SetPortValueQueuedEvent.hpp \
	events/UnregisterClientEvent.cpp \
	events/UnregisterClientEvent.hpp \
	jack_compat.h \
	tuning.hpp \
	types.hpp \
	util.hpp

if WITH_LADSPA
libingen_engine_la_SOURCES += \
	LADSPANode.hpp \
	LADSPANode.cpp
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

