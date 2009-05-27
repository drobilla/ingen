/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "ingen-config.h"
#include "raul/SharedPtr.hpp"
#include "raul/AtomLiblo.hpp"
#include "interface/ClientInterface.hpp"
#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "OSCClientSender.hpp"
#include "OSCEngineReceiver.hpp"
#include "QueuedEventSource.hpp"
#include "ThreadManager.hpp"

using namespace std;

namespace Ingen {


/*! \page engine_osc_namespace Engine OSC Namespace Documentation
 *
 * <p>These are the commands the engine recognizes.  A client can control every
 * aspect of the engine entirely with these commands.</p>
 *
 * <p>All commands on this page are in the "control band".  If a client needs to
 * know about the state of the engine, it must listen to the "notification band".
 * See the "Client OSC Namespace Documentation" for details.
 */


OSCEngineReceiver::OSCEngineReceiver(Engine& engine, size_t queue_size, uint16_t port)
	: QueuedEngineInterface(engine, queue_size) // FIXME
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
	//lo_server_add_method(_server, NULL, NULL, generic_cb, NULL);

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
	lo_server_add_method(_server, "/ingen/put", NULL, put_cb, this);
	lo_server_add_method(_server, "/ingen/move", "iss", move_cb, this);
	lo_server_add_method(_server, "/ingen/delete", "is", del_cb, this);
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
OSCEngineReceiver::activate_source()
{
	QueuedEventSource::activate_source();
	_receive_thread->set_name("OSC Receiver");
	_receive_thread->start();
	_receive_thread->set_scheduling(SCHED_FIFO, 5); // Jack default appears to be 10
}


void
OSCEngineReceiver::deactivate_source()
{
	_receive_thread->stop();
	QueuedEventSource::deactivate_source();
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
		Shared::ClientInterface* client = me->_engine.broadcaster()->client(url);
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
 * <h2>/ingen/ping</h2>
 * \arg \b response-id (integer)
 *
 * Reply to sender immediately with a successful response.
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
 * <h2>/ingen/ping_queued</h2>
 * \arg \b response-id (integer)
 *
 * Reply to sender with a successful response after going through the event queue.
 * This is useful for checking if the engine is actually active, or for sending after
 * several events as a sentinel and wait on it's response, to know when all previous
 * events have finished processing.
 */
int
OSCEngineReceiver::_ping_slow_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	ping();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/quit</h2>
 * \arg \b response-id (integer)
 *
 * Terminate the engine.
 * Note that there are NO order guarantees with this command at all.  You could
 * send 10 messages then quit, and the quit reply could come immediately and the
 * 10 messages would never get executed.
 */
int
OSCEngineReceiver::_quit_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	quit();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/register_client</h2>
 * \arg \b response-id (integer)
 *
 * Register a new client with the engine.
 * The incoming address will be used for the new registered client.  If you
 * want to register a different specific address, use the URL version.
 */
int
OSCEngineReceiver::_register_client_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	lo_address addr = lo_message_get_source(msg);

	char* const url = lo_address_get_url(addr);
	Shared::ClientInterface* client = new OSCClientSender((const char*)url);
	register_client(client);
	free(url);

	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/unregister_client</h2>
 * \arg \b response-id (integer)
 *
 * Unregister a client.
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
 * <h2>/ingen/load_plugins</h2>
 * \arg \b response-id (integer)
 *
 * Locate all available plugins, making them available for use.
 */
int
OSCEngineReceiver::_load_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	load_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/activate</h2>
 * \arg \b response-id (integer)
 *
 * Activate the engine (event processing and all drivers, e.g. audio and MIDI).
 * Note that you <b>must</b> send this message first if you want the engine to do
 * anything at all - <em>including respond to your messages!</em>
 */
int
OSCEngineReceiver::_engine_activate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::activate();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/deactivate</h2>
 * \arg \b response-id (integer)
 *
 * Deactivate the engine.
 */
int
OSCEngineReceiver::_engine_deactivate_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	QueuedEngineInterface::deactivate();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/put</h2>
 * \arg \b response-id (integer)
 * \arg \b path (string) - Path of object
 * \arg \b predicate
 * \arg \b value
 * \arg \b ...
 *
 * PUT a set of properties to a path (see \ref methods).
 */
int
OSCEngineReceiver::_put_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	//const char* path = &argv[1]->s;

	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/move</h2>
 * \arg \b response-id (integer)
 * \arg \b old-path - Object's path
 * \arg \b new-path - Object's new path
 *
 * MOVE an object to a new path (see \ref methods).
 */
int
OSCEngineReceiver::_move_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* old_path = &argv[1]->s;
	const char* new_path = &argv[2]->s;

	move(old_path, new_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/clear_patch</h2>
 * \arg \b response-id (integer)
 * \arg \b patch-path - Patch's path
 *
 * Remove all nodes from a patch.
 */
int
OSCEngineReceiver::_clear_patch_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path = &argv[1]->s;

	clear_patch(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/del</h2>
 * \arg \b response-id (integer)
 * \arg \b path (string) - Full path of the object
 *
 * DELETE an object (see \ref methods).
 */
int
OSCEngineReceiver::_del_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* obj_path = &argv[1]->s;

	del(obj_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/connect</h2>
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port
 *
 * Connect two ports (which must be in the same patch).
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
 * <h2>/ingen/disconnect</h2>
 * \arg \b response-id (integer)
 * \arg \b src-port-path (string) - Full path of source port
 * \arg \b dst-port-path (string) - Full path of destination port
 *
 * Disconnect two ports.
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
 * <h2>/ingen/disconnect_all</h2>
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - The (parent) patch in which to disconnect object.
 * \arg \b object-path (string) - Full path of object.
 *
 * Disconnect all connections to/from a node/port.
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
 * <h2>/ingen/set_port_value</h2>
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Name of port
 * \arg \b value (float) - Value to set port to.
 *
 * Set the value of a port for all voices.
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
 * <h2>/ingen/note_on</h2>
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)
 * \arg \b velocity (int) - MIDI style velocity (0-127)
 *
 * Trigger a note-on, just as if it came from MIDI.
 */
int
OSCEngineReceiver::_note_on_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*
	const char* node_path   = &argv[1]->s;
	const uint8_t note_num    =  argv[2]->i;
	const uint8_t velocity    =  argv[3]->i;
	*/
	cerr << "FIXME: OSC note on\n";
	//note_on(node_path, note_num, velocity);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/note_off</h2>
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Patch of Node to trigger (must be a trigger or note node)
 * \arg \b note-num (int) - MIDI style note number (0-127)
 *
 * Trigger a note-off, just as if it came from MIDI.
 */
int
OSCEngineReceiver::_note_off_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	/*
	const char* patch_path  = &argv[1]->s;
	const uint8_t note_num    =  argv[2]->i;
	*/
	cerr << "FIXME: OSC note off\n";
	//note_off(patch_path, note_num);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/all_notes_off</h2>
 * \arg \b response-id (integer)
 * \arg \b patch-path (string) - Patch of patch to send event to
 *
 * Trigger a note-off for all voices, just as if it came from MIDI.
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
 * <h2>/ingen/midi_learn</h2>
 * \arg \b response-id (integer)
 * \arg \b node-path (string) - Path of control node.
 *
 * Initiate MIDI learn for a given control node.
 * The node will learn the next MIDI control event it receives and set
 * its outputs accordingly.
 * This command does nothing for objects that are not a control internal.
 */
int
OSCEngineReceiver::_midi_learn_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* patch_path  = &argv[1]->s;

	midi_learn(patch_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/set_variable</h2>
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object to associate variable with
 * \arg \b key (string) - Key (index/predicate/ID) for new variable
 * \arg \b value (string) - Value of new variable
 *
 * Set a variable, associated with a graph object.
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
 * <h2>/ingen/set_property</h2>
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object to associate variable with
 * \arg \b key (string) - URI/QName for predicate of this property (e.g. "ingen:enabled")
 * \arg \b value (string) - Value of property
 *
 * Set a property on a graph object.
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
 * <h2>/ingen/request_variable</h2>
 * \arg \b response-id (integer)
 * \arg \b object-path (string) - Full path of object variable is associated with
 * \arg \b key (string) - Key (index) for piece of variable
 *
 * Request the value of a variable on a graph object.
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
 * <h2>/ingen/request_plugin</h2>
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Full path of port to send the value of
 *
 * Request the value of a port.
 */
int
OSCEngineReceiver::_request_plugin_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* uri = &argv[1]->s;

	request_plugin(uri);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/request_object</h2>
 * \arg \b response-id (integer)
 * \arg \b port-path (string) - Full path of port to send the value of \n\n
 *
 * Request all properties of a graph object.
 */
int
OSCEngineReceiver::_request_object_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	const char* object_path = &argv[1]->s;

	request_object(object_path);
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/request_plugins</h2>
 * \arg \b response-id (integer)
 *
 * Request the engine send a list of all known plugins.
 */
int
OSCEngineReceiver::_request_plugins_cb(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg)
{
	request_plugins();
	return 0;
}


/** \page engine_osc_namespace
 * <h2>/ingen/request_all_objects</h2>
 * \arg \b response-id (integer)
 *
 * Requests all information about all known objects.
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
