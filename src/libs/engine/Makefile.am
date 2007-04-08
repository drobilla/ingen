SUBDIRS = tests events

MAINTAINERCLEANFILES = Makefile.in

noinst_LTLIBRARIES = libingen.la
libingen_la_CXXFLAGS = @RAUL_CFLAGS@ @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @SLV2_CFLAGS@ -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs/engine/events

libingen_la_LIBADD = @RAUL_LIBS@ @JACK_LIBS@ @LOSC_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @SLV2_LIBS@

libingen_la_SOURCES = \
	util.h \
	tuning.h \
	events.h \
	ThreadManager.h \
	DataType.h \
	Node.h \
	NodeBase.h \
	NodeBase.cpp \
	InternalNode.h \
	Patch.h \
	Patch.cpp \
	NodeFactory.h \
	NodeFactory.cpp \
	Engine.h \
	Engine.cpp \
	JackAudioDriver.h \
	JackAudioDriver.cpp \
	OSCEngineReceiver.h \
	OSCEngineReceiver.cpp \
	Responder.h \
	DirectResponder.h \
	OSCResponder.h \
	OSCResponder.cpp \
	ClientBroadcaster.h \
	ClientBroadcaster.cpp \
	ObjectSender.h \
	ObjectSender.cpp \
	OSCClientSender.h \
	OSCClientSender.cpp \
	Buffer.h \
	AudioBuffer.h \
	AudioBuffer.cpp \
	MidiBuffer.h \
	MidiBuffer.cpp \
	BufferFactory.cpp \
	Port.h \
	Port.cpp \
	InputPort.h \
	InputPort.cpp \
	OutputPort.h \
	DuplexPort.h \
	DuplexPort.cpp \
	MidiMessage.h \
	MidiNoteNode.h \
	MidiNoteNode.cpp \
	MidiTriggerNode.h \
	MidiTriggerNode.cpp \
	MidiControlNode.h \
	MidiControlNode.cpp \
	Event.h \
	QueuedEvent.h \
	EventSource.h \
	QueuedEventSource.h \
	QueuedEventSource.cpp \
	QueuedEngineInterface.h \
	QueuedEngineInterface.cpp \
	GraphObject.h \
	GraphObject.cpp \
	Tree.h \
	TreeImplementation.h \
	PluginLibrary.h \
	Plugin.h \
	Plugin.cpp \
	PostProcessor.h \
	PostProcessor.cpp \
	Connection.h \
	Connection.cpp \
	ObjectStore.h \
	ObjectStore.cpp \
	TransportNode.h \
	TransportNode.cpp \
	Driver.h \
	AudioDriver.h \
	MidiDriver.h \
	instantiations.cpp \
	events/RegisterClientEvent.h \
	events/RegisterClientEvent.cpp \
	events/UnregisterClientEvent.h \
	events/UnregisterClientEvent.cpp \
	events/PingQueuedEvent.h \
	events/DeactivateEvent.h \
	events/DeactivateEvent.cpp \
	events/SetPortValueEvent.h \
	events/SetPortValueEvent.cpp \
	events/SetPortValueQueuedEvent.h \
	events/SetPortValueQueuedEvent.cpp \
	events/NoteOnEvent.h \
	events/NoteOnEvent.cpp \
	events/NoteOffEvent.h \
	events/NoteOffEvent.cpp \
	events/AllNotesOffEvent.h \
	events/AllNotesOffEvent.cpp \
	events/ConnectionEvent.h \
	events/ConnectionEvent.cpp \
	events/DisconnectionEvent.h \
	events/DisconnectionEvent.cpp \
	events/DisconnectNodeEvent.h \
	events/DisconnectNodeEvent.cpp \
	events/DisconnectPortEvent.h \
	events/DisconnectPortEvent.cpp \
	events/DestroyEvent.h \
	events/DestroyEvent.cpp \
	events/AddPortEvent.h \
	events/AddPortEvent.cpp \
	events/AddNodeEvent.h \
	events/AddNodeEvent.cpp \
	events/SetMetadataEvent.h \
	events/SetMetadataEvent.cpp \
	events/RequestMetadataEvent.h \
	events/RequestMetadataEvent.cpp \
	events/RequestPluginEvent.h \
	events/RequestPluginEvent.cpp \
	events/RequestObjectEvent.h \
	events/RequestObjectEvent.cpp \
	events/RequestPortValueEvent.h \
	events/RequestPortValueEvent.cpp \
	events/RequestAllObjectsEvent.h \
	events/RequestAllObjectsEvent.cpp \
	events/RequestPluginsEvent.h \
	events/RequestPluginsEvent.cpp \
	events/CreatePatchEvent.h \
	events/CreatePatchEvent.cpp \
	events/LoadPluginsEvent.h \
	events/LoadPluginsEvent.cpp \
	events/EnablePatchEvent.h \
	events/EnablePatchEvent.cpp \
	events/DisablePatchEvent.h \
	events/DisablePatchEvent.cpp \
	events/ClearPatchEvent.h \
	events/ClearPatchEvent.cpp \
	events/RenameEvent.h \
	events/RenameEvent.cpp \
	events/MidiLearnEvent.h \
	events/MidiLearnEvent.cpp \
	types.h
# 	BridgeNode.h 
# 	BridgeNode.cpp 
# 	ControlInputNode.h 
# 	ControlInputNode.cpp 
# 	ControlOutputNode.h 
# 	ControlOutputNode.cpp 
# 	AudioInputNode.h 
# 	AudioInputNode.cpp 
# 	AudioOutputNode.h 
# 	AudioOutputNode.cpp 
# 	MidiInputNode.h 
# 	MidiInputNode.cpp 
# 	MidiOutputNode.h 
# 	MidiOutputNode.cpp 

if WITH_JACK_MIDI
libingen_la_SOURCES += \
	JackMidiDriver.h \
	JackMidiDriver.cpp
endif

if WITH_ALSA_MIDI
libingen_la_SOURCES += \
	AlsaMidiDriver.h \
	AlsaMidiDriver.cpp
endif

if WITH_LADSPA
libingen_la_SOURCES += \
	LADSPANode.h \
	LADSPANode.cpp
endif

if WITH_DSSI
libingen_la_SOURCES += \
	DSSINode.h \
	DSSINode.cpp \
	events/DSSIConfigureEvent.cpp \
	events/DSSIConfigureEvent.h \
	events/DSSIControlEvent.cpp \
	events/DSSIControlEvent.h \
	events/DSSIProgramEvent.cpp \
	events/DSSIProgramEvent.h \
	events/DSSIUpdateEvent.cpp \
	events/DSSIUpdateEvent.h
endif

if WITH_LV2
libingen_la_SOURCES += \
	LV2Node.h \
	LV2Node.cpp
endif

if WITH_LASH
libingen_la_SOURCES += \
	LashDriver.h \
	LashDriver.cpp
endif

