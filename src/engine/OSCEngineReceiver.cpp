/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

//#define ENABLE_AVAHI 1

#include <iostream>
#include <cstdlib>
#include <string>
#include <lo/lo.h>
#include "raul/SharedPtr.hpp"
#include "raul/AtomLiblo.hpp"
#include "types.hpp"
#include "interface/ClientInterface.hpp"
#include "engine/ThreadManager.hpp"
#include "OSCEngineReceiver.hpp"
#include "QueuedEventSource.hpp"
#include "OSCClientSender.hpp"
#include "ClientBroadcaster.hpp"

using namespace std;

namespace Ingen {


/*! \page engine_osc_namespace Engine OSC Namespace Documentation
 *
 * <p>These are the commands the engine recognizes.  A client can control every
 * aspect of the engine entirely with these commands.</p>
 *
 * <p>All commands on this page are in the "control band".  If a client needs to
 * know about the state of the engine, it must listen to the "notification band".
 * See the "Client OSC Namespace Documentation" for details.</p>\n\n
 */


OSCEngineReceiver::OSCEngineReceiver(Engine& engine, size_t queue_size, uint16_t port)
	: QueuedEngineInterface(engine, queue_size, queue_size) // FIXME
	, _server(NULL)
{
	_receive_thread = new ReceiveThread(*this);

	char port_str[6];
	snprintf(port_str, 6, "%u", port);

	_server = lo_server_new(port_str, error_cb);
#ifdef ENABLE_AVAHI
	lo_server_avahi_init(_server, "ingen");
#endif
	
	if (_server == NULL) {
		cerr << "[OSC] Could not start OSC server.  Aborting." << endl;
		exit(EXIT_FAILURE);
	} else {
		char* lo_url = lo_server_get_url(_server);
		cout << "[OSC] Started OSC server at " << lo_url << endl;
		free(lo_url);
	}

	// For debugging, print all incoming OSC messages
	lo_server_add_method(_server, NULL, NULL, generic_cb, NULL);

	// Set response address for this message.
	// It's important this is first and returns nonzero.
	lo_server_add_method(_server, NULL, NULL, set_response_address_cb, this);
	
	// Commands
	lo_server_add_method(_server, "/ingen/ping", "i", ping_cb, this);
	lo_server_add_method(_server, "/ingen/ping_queued", "i", ping_slow_cb, this);
	lo_server_add_method(_server, "/ingen/quit", "i", quit_cb, this);
	//lo_server_add_method(_server, "/ingen/register_client", "is", register_client_cb, this);
	lo_server_add_method(_server, "/ingen/register_client", "i", register_client_cb, this);
	lo_server_add_method(_server, "/ingen/unregister_client", "i", unregister_client_cb, this);
	lo_server_add_method(_server, "/ingen/load_plugins", "i", load_plugins_cb, this);
	lo_server_add_method(_server, "/ingen/activate", "i", engine_activate_cb, this);
	lo_server_add_method(_server, "/ingen/deactivate", "i", engine_deactivate_cb, this);
	lo_server_add_method(_server, "/ingen/new_patch", "isi", new_patch_cb, this);
	lo_server_add_method(_server, "/ingen/clear_patch", "is", clear_patch_cb, this);
	lo_server_add_method(_server, "/ingen/set_polyphony", "isi", set_polyphony_cb, this);
	lo_server_add_method(_server, "/ingen/set_polyphonic", "isT", set_polyphonic_cb, this);
	lo_server_add_method(_server, "/ingen/set_polyphonic", "isF", set_polyphonic_cb, this);
	lo_server_add_method(_server, "/ingen/new_port", "issi", new_port_cb, this);
	lo_server_add_method(_server, "/ingen/new_node", "issss", new_node_cb, this);
	lo_server_add_method(_server, "/ingen/new_node", "issss", new_node_cb, this);
	lo_server_add_method(_server, "/ingen/new_node", "iss", new_node_by_uri_cb, this);
	lo_server_add_method(_server, "/ingen/new_node", "iss", new_node_by_uri_cb, this);
	lo_server_add_method(_server, "/ingen/destroy", "is", destroy_cb, this);
	lo_server_add_method(_server, "/ingen/rename", "iss", rename_cb, this);
	lo_server_add_method(_server, "/ingen/connect", "iss", connect_cb, this);
	lo_server_add_method(_server, "/ingen/disconnect", "iss", disconnect_cb, this);
	lo_server_add_method(_server, "/ingen/disconnect_all", "iss", disconnect_all_cb, this);
	lo_server_add_method(_server, "/ingen/set_port_value", NULL, set_port_value_cb, this);
	lo_server_add_method(_server, "/ingen/note_on", "isii", note_on_cb, this);
	lo_server_add_method(_server, "/ingen/note_off", "isi", note_off_cb, this);
	lo_server_add_method(_server, "/ingen/all_notes_off", "isi", all_notes_off_cb, this);
	lo_server_add_method(_server, "/ingen/midi_learn", "is", midi_learn_cb, this);
	lo_server_add_method(_server, "/ingen/set_variable", NULL, variable_set_cb, this);
	lo_server_add_method(_server, "/ingen/set_property", NULL, property_set_cb, this);

	// Queries
	lo_server_add_method(_server, "/ingen/request_variable", "iss", variable_get_cb, this);
	lo_server_add_method(_server, "/ingen/request_plugin", "is", request_plugin_cb, this);
	lo_server_add_method(_server, "/ingen/request_object", "is", request_object_cb, this);
	lo_server_add_method(_server, "/ingen/request_port_value", "is", request_port_value_cb, this);
	lo_server_add_method(_server, "/ingen/request_plugins", "i", request_plugins_cb, this);
	lo_server_add_method(_server, "/ingen/request_all_objects", "i", request_all_objects_cb, this);

	lo_server_add_method(_server, NULL, NULL, unknown_cb, NULL);

	Thread::set_name("OSC Receiver");
}


OSCEngineReceiver::~OSCEngineReceiver()
{
	deactivate();
	stop();
	_receive_thread->stop();
	delete _receive_thread;

	if (_server != NULL)  {
#ifdef ENABLE_AVAHI
		lo_server_avahi_free(_server);
#endif
		lo_server_free(_server);
		_server = NULL;
	}
}


void
OSCEngineReceiver::activate()
{
	QueuedEventSource::activate();
	_receive_thread->set_name("OSC Receiver");
	_receive_thread->start();
	_receive_thread->set_scheduling(SCHED_FIFO, 5); // Jack default appears to be 10
}


void
OSCEngineReceiver::deactivate()
{
	cout << "[OSCEngineReceiver] Stopped OSC listening thread" << endl;
	_receive_thread->stop();
	QueuedEventSource::deactivate();
}


/** Override the semaphore driven _run method of QueuedEngineInterface
 * to wait on OSC messages and prepare them right away in the same thread.
 */
void
OSCEngineReceiver::ReceiveThread::_run()
{
	Thread::get().set_context(THREAD_PRE_PROCESS);

	/* get a timestamp here and stamp all the events with the same time so
	 * they all get executed in the same cycle */

	while (true) {
		assert(_receiver._server);
		/*if ( ! _server) {
			cout << "[OSCEngineReceiver] Server is NULL, exiting" << endl;
			break;
		}*/

		// Wait on a message and enqueue it
		lo_server_recv(_receiver._server);

		// Enqueue every other message that is here "now"
		// (would this provide truly atomic bundles?)
		while (lo_server_recv_noblock(_receiver._server, 0) > 0) 
			if (_receiver.unprepared_events())
				_receiver.whip();

		// No more unprepared events
	}	
}


/** Create a new responder for this message, if necessary.
 *
 * This is based on the fact that the current responder is stored in a ref
 * counted pointer, and events just take a reference to that.  Thus, events
 * may delete their responder if we've since switched to a new one, or the
 * same one can stay around and serve a series of events.
 * Hooray for reference counting.
 *
 * If this message came from the same source as the last message, no allocation
 * of responders or lo_addresses or any of it needs to be done.  Unfortunately
 * the only way to check is by comparing URLs, because liblo addresses suck.
 * Lack of a fast liblo address comparison really sucks here, in any case.
 */
int
OSCEngineReceiver::set_response_address_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	OSCEngineReceiver* const me = reinterpret_cast<OSCEngineReceiver*>(user_data);

	if (argc < 1 || types[0] != 'i') // Not a valid Ingen message
		return 0; // Save liblo the trouble

	const int32_t id = argv[0]->i;

	const lo_address addr = lo_message_get_source(msg);
	char* const      url  = lo_address_get_url(addr);
		
	const SharedPtr<Responder> r = me->_responder;

	/* Different address than last time, have to do a lookup */
	if (!r || !r->client() || strcmp(url, r->client()->uri().c_str())) {
		ClientInterface* client = me->_engine.broadcaster()->client(url);
		if (client)
			me->_responder = SharedPtr<Responder>(new Responder(client, id));
		else
			me->_responder = SharedPtr<Responder>(new Responder());
	}
	
	if (id != -1) {
		me->set_next_response_id(id);
	} else {
		me->disable_responses();
	}

	// If this returns 0 no OSC commands will work
	return 1;
}


void
OSCEngineReceiver::error_cb(int num, const char* msg, const char* path)
{
	cerr << "liblo server error " << num << " in path \"" << "\" - " << msg << endl;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/ping - Immediately sends a successful response to the given response id.
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::_ping_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const lo_address addr = lo_message_get_source(msg);
	if (lo_send(addr, "/ingen/ok", "i", argv[0]->i) < 0)
		cerr << "WARNING: Unable to send response: " << lo_address_errstr(addr) << endl;
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/ping_queued - Sends response after going through the event queue.
 * \arg \b response-id (integer)
 *
 * \li See the documentation for /ingen/set_port_value_queued for an explanation of how
 * this differs from /ingen/ping.  This is useful to send after sending a large cluster of
 * events as a sentinel and wait on it's response, to know when the events are all
 * finished processing.</p> \n \n
 */
int
OSCEngineReceiver::_ping_slow_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	ping();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/quit - Terminates the engine.
 * \arg \b response-id (integer)
 *
 * \li Note that there is NO order guarantees with this command at all.  You could
 * send 10 messages then quit, and the quit reply could come immediately and the
 * 10 messages would never get executed. </p> \n \n
 */
int
OSCEngineReceiver::_quit_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	quit();
	return 0;
}

/** \page engine_osc_namespace
 * <p> \b /ingen/register_client - Registers a new client with the engine
 * \arg \b response-id (integer) \n\n
 * \li The incoming address will be used for the new registered client.  If you
 * want to register a different specific address, use the URL version. </p> \n \n
 */
int
OSCEngineReceiver::_register_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address addr = lo_message_get_source(msg);

	char* const url = lo_address_get_url(addr);
	ClientInterface* client = new OSCClientSender((const char*)url);
	register_client(client);
	free(url);

	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/unregister_client - Unregisters a client
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::_unregister_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address addr = lo_message_get_source(msg);

	char* url = lo_address_get_url(addr);
	unregister_client(url);
	free(url);
	
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/load_plugins - Locates all available plugins, making them available for use.
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::_load_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	load_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/activate - Activate the engine (MIDI, audio, everything)
 * \arg \b response-id (integer) </p>
 *
 * \li Note that you <b>must</b> send this message first if you want the engine to do
 * anything at all - <em>including respond to your messages!</em> \n \n
 */
int
OSCEngineReceiver::_engine_activate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::activate();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/deactivate - Deactivate the engine completely.
 * \arg \b response-id (integer) </p> \n \n
 */
int
OSCEngineReceiver::_engine_deactivate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::deactivate();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/new_patch - Creates a new, empty, toplevel patch.
 * \arg \b response-id  (integer)
 * \arg \b patch-path  (string) - Patch path (complete, ie /master/parent/new_patch)
 * \arg \b poly        (integer) - Patch's (internal) polyphony </p> \n \n
 */
int
OSCEngineReceiver::_new_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char*   patch_path  = &argv[1]->s;
	const int32_t poly        =  argv[2]->i;

	new_patch(patch_path, poly);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/rename - Rename an Object (only Nodes, for now)
 * \arg \b response-id (integer)
 * \arg \b path - Object's path
 * \arg \b name - New name for object </p> \n \n
 */
int
OSCEngineReceiver::_rename_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path  = &argv[1]->s;
	const char* name         = &argv[2]->s;
	
	rename(object_path, name);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/clear_patch - Remove all nodes from a patch
 * \arg \b response-id (integer)
 * \arg \b patch-path - Patch's path </p> \n \n
 */
int
OSCEngineReceiver::_clear_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	
	clear_patch(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/set_polyphony - Set the polyphony of a patch
 * \arg \b response-id (integer)
 * \arg \b patch-path - Patch's path
 * \arg \b poly (integer) </p> \n \n
 */
int
OSCEngineReceiver::_set_polyphony_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char*    patch_path = &argv[1]->s;
	const uint32_t poly       = argv[2]->i;
	
	set_polyphony(patch_path, poly);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/set_polyphonic - Toggle a node's or port's polyphonic mode
 * \arg \b response-id (integer)
 * \arg \b path - Object's path
 * \arg \b polyphonic (bool) </p> \n \n
 */
int
OSCEngineReceiver::_set_polyphonic_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path = &argv[1]->s;
	bool        polyphonic  = (types[2] == 'T');
	
	set_polyphonic(object_path, polyphonic);
	return 0;
}


// FIXME: add index
/** \page engine_osc_namespace
 * <p> \b /ingen/new_port - Add a port into a given patch (load a plugin by URI)
 * \arg \b response-id (integer)
 * \arg \b path (string) - Full path of the new port (ie. /patch2/subpatch/newport)
 * \arg \b data-type (string) - Type of port (lv2:AudioPort, lv2:ControlPort, lv2ev:EventPort)
 * \arg \b direction ("is-output") (integer) - Direction of data flow (Input = 0, Output = 1) </p> \n \n
 */
int
OSCEngineReceiver::_new_port_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char*   port_path   = &argv[1]->s;
	const char*   data_type   = &argv[2]->s;
	const int32_t direction   =  argv[3]->i;
	
	new_port(port_path, 0, data_type, (direction == 1));
	return 0;
}

/** \page engine_osc_namespace
 * <p> \b /ingen/new_node - Add a node into a given patch (load a plugin by URI)
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of the new node (ie. /patch2/subpatch/newnode)
 * \arg \b plug-uri (string) - URI of the plugin to load \n \n
 */
int
OSCEngineReceiver::_new_node_by_uri_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path  = &argv[1]->s;
	const char* plug_uri   = &argv[2]->s;
	
	new_node(node_path, plug_uri);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/new_node - Add a node into a given patch (load a plugin by libname, label) \b DEPRECATED
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of the new node (ie. /patch2/subpatch/newnode)
 * \arg \b type (string) - Plugin type ("LADSPA" or "Internal")
 * \arg \b lib-name (string) - Name of library where plugin resides (eg "cmt.so")
 * \arg \b plug-label (string) - Label (ID) of plugin (eg "sine_fcaa") \n \n
 *
 * \li This is only here to provide backwards compatibility for old patches that store LADSPA plugin
 * references as libname, label.  It is to be removed ASAP, don't use it.
 * </p> \n \n
 */
int
OSCEngineReceiver::_new_node_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;
	const char* type        = &argv[2]->s;
	const char* lib_name    = &argv[3]->s;
	const char* plug_label  = &argv[4]->s;
	
	new_node_deprecated(node_path, type, lib_name, plug_label);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/destroy - Removes (destroys) a Patch or a Node
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Full path of the object </p> \n \n
 */
int
OSCEngineReceiver::_destroy_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* node_path   = &argv[1]->s;
	
	destroy(node_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/connect - Connects two ports (must be in the same patch)
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port </p> \n \n
 */
int
OSCEngineReceiver::_connect_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[1]->s;
	const char* dst_port_path = &argv[2]->s;

	connect(src_port_path, dst_port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/disconnect - Disconnects two ports.
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port </p> \n \n
 */
int
OSCEngineReceiver::_disconnect_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* src_port_path = &argv[1]->s;
	const char* dst_port_path = &argv[2]->s;

	disconnect(src_port_path, dst_port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/disconnect_all - Disconnect all connections to/from a node/port.
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - The (parent) patch in which to disconnect object. </p> \n \n
 * \arg \b node-path (string) - Full path of object. </p> \n \n
 */
int
OSCEngineReceiver::_disconnect_all_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path = &argv[1]->s;
	const char* object_path = &argv[2]->s;

	disconnect_all(patch_path, object_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/set_port_value - Sets the value of a port for all voices (as a QueuedEvent)
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Name of port
 * \arg \b value (float) - Value to set port to.</p> \n \n
 */
int
OSCEngineReceiver::_set_port_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc < 3 || argc > 5 || strncmp(types, "is", 2))
		return 1;
	
	const char* port_path = &argv[1]->s;

	using Raul::Atom;

	if (!strcmp(types, "isf")) { // float, all voices
		const float value = argv[2]->f;
		set_port_value(port_path, Atom(value));
	} else if (!strcmp(types, "isif")) { // float, specific voice
		const float value = argv[3]->f;
		set_voice_value(port_path, argv[2]->i, Atom(value));
	} else if (!strcmp(types, "issb")) { // blob (event), all voices
		const char* type = &argv[2]->s;
		lo_blob b = argv[3];
		size_t data_size = lo_blob_datasize(b);
		void* data = lo_blob_dataptr(b);
		set_port_value(port_path, Atom(type, data_size, data));
	} else if (!strcmp(types, "isisb")) { // blob (event), specific voice
		const char* type = &argv[3]->s;
		lo_blob b = argv[4];
		size_t data_size = lo_blob_datasize(b);
		void* data = lo_blob_dataptr(b);
		set_voice_value(port_path, argv[2]->i, Atom(type, data_size, data));
	} else if (!strcmp(types, "issN")) { // empty event (type only), all voices
		const char* type = &argv[2]->s;
		set_port_value(port_path, Atom(type, 0, NULL));
	} else if (!strcmp(types, "isisN")) { // empty event (type only), specific voice
		const char* type = &argv[3]->s;
		set_voice_value(port_path, argv[2]->i, Atom(type, 0, NULL));
	} else {
		return 1;
	}

	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/note_on - Triggers a note-on, just as if it came from MIDI
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)
 * \arg \b velocity (int) - MIDI style velocity (0-127)</p> \n \n
 */
int
OSCEngineReceiver::_note_on_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* node_path   = &argv[1]->s;
	const uchar note_num    =  argv[2]->i;
	const uchar velocity    =  argv[3]->i;
	*/
	cerr << "FIXME: OSC note on\n";
	//note_on(node_path, note_num, velocity);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/note_off - Triggers a note-off, just as if it came from MIDI
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)</p> \n \n
 */
int
OSCEngineReceiver::_note_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* patch_path  = &argv[1]->s;
	const uchar note_num    =  argv[2]->i;
	*/
	cerr << "FIXME: OSC note off\n";
	//note_off(patch_path, note_num);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/all_notes_off - Triggers a note-off for all voices, just as if it came from MIDI
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - Patch of patch to send event to </p> \n \n
 */
int
OSCEngineReceiver::_all_notes_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*

	const char* patch_path  = &argv[1]->s;
	*/
	cerr << "FIXME: OSC all notes off\n";
	//all_notes_off(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/midi_learn - Initiate MIDI learn for a given (MIDI Control) Node
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of the Node that should learn the next MIDI event.
 * 
 * \li This of course will only do anything for MIDI control nodes.  The node will learn the next MIDI
 * event that arrives at it's MIDI input port - no behind the scenes voodoo happens here.  It is planned
 * that a plugin specification supporting arbitrary OSC commands for plugins will exist one day, and this
 * method will go away completely. </p> \n \n
 */
int
OSCEngineReceiver::_midi_learn_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;
	
	midi_learn(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/set_variable - Set a variable, associated with a synth-space object (node, etc)
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object to associate variable with
 * \arg \b key (string) - Key (index/predicate/ID) for new variable
 * \arg \b value (string) - Value of new variable </p> \n \n
 */
int
OSCEngineReceiver::_variable_set_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc != 4 || types[0] != 'i' || types[1] != 's' || types[2] != 's')
		return 1;

	const char* object_path = &argv[1]->s;
	const char* key         = &argv[2]->s;
	
	Raul::Atom value = Raul::AtomLiblo::lo_arg_to_atom(types[3], argv[3]);
	
	set_variable(object_path, key, value);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/set_property - Set an (RDF) property, associated with a synth-space object (node, etc)
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object to associate variable with
 * \arg \b key (string) - URI/QName for predicate of this property (e.g. "ingen:enabled")
 * \arg \b value (string) - Value of property </p> \n \n
 */
int
OSCEngineReceiver::_property_set_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	if (argc != 4 || types[0] != 'i' || types[1] != 's' || types[2] != 's')
		return 1;

	const char* object_path = &argv[1]->s;
	const char* key         = &argv[2]->s;
	
	Raul::Atom value = Raul::AtomLiblo::lo_arg_to_atom(types[3], argv[3]);
	
	set_property(object_path, key, value);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/request_variable - Requests the engine send a piece of variable, associated with a synth-space object (node, etc)
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object variable is associated with
 * \arg \b key (string) - Key (index) for piece of variable
 *
 * \li Reply will be sent to client registered with the source address of this message.</p> \n \n
 */
int
OSCEngineReceiver::_variable_get_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path = &argv[1]->s;
	const char* key         = &argv[2]->s;

	request_variable(object_path, key);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/request_plugin - Requests the engine send the value of a port.
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Full path of port to send the value of \n\n
 * \li Reply will be sent to client registered with the source address of this message.</p>\n\n
 */
int
OSCEngineReceiver::_request_plugin_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* uri = &argv[1]->s;

	request_plugin(uri);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/request_object - Requests the engine send the value of a port.
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Full path of port to send the value of \n\n
 * \li Reply will be sent to client registered with the source address of this message.</p>\n\n
 */
int
OSCEngineReceiver::_request_object_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path = &argv[1]->s;

	request_object(object_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/request_port_value - Requests the engine send the value of a port.
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Full path of port to send the value of \n\n
 * \li Reply will be sent to client registered with the source address of this message.</p>\n\n
 */
int
OSCEngineReceiver::_request_port_value_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* port_path = &argv[1]->s;

	request_port_value(port_path);
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/request_plugins - Requests the engine send a list of all known plugins.
 * \arg \b response-id (integer) \n\n
 * \li Reply will be sent to client registered with the source address of this message.</p>\n\n
 */
int
OSCEngineReceiver::_request_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	request_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <p> \b /ingen/request_all_objects - Requests the engine send information about \em all objects (patches, nodes, etc)
 * \arg \b response-id (integer)\n\n
 * \li Reply will be sent to client registered with the source address of this message.</p> \n \n
 */
int
OSCEngineReceiver::_request_all_objects_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	request_all_objects();
	return 0;
}


//  Static Callbacks //


// Display incoming OSC messages (for debugging purposes)
int
OSCEngineReceiver::generic_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	printf("[OSCMsg] %s (%s)\t", path, types);
    
	for (int i=0; i < argc; ++i) {
		lo_arg_pp(lo_type(types[i]), argv[i]);
		printf("\t");
    }
    printf("\n");

	return 1;  // not handled
}


int
OSCEngineReceiver::unknown_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	const lo_address addr = lo_message_get_source(msg);
	char* const      url  = lo_address_get_url(addr);

	cerr << "Unknown command " << path << " (" << types << "), sending error.\n";

	string error_msg = "Unknown command: ";
	error_msg.append(path).append(" ").append(types);

	OSCClientSender(url).error(error_msg);

	return 0;
}


} // namespace Ingen


extern "C" {

Ingen::OSCEngineReceiver*
new_osc_receiver(Ingen::Engine& engine, size_t queue_size, uint16_t port)
{
	return new Ingen::OSCEngineReceiver(engine, queue_size, port);
}

} // extern "C"
