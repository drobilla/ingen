/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "QueuedEngineInterface.h"
#include "QueuedEventSource.h"
#include "events.h"
#include "Om.h"
#include "util/Queue.h"
#include "OmApp.h"

namespace Om {

QueuedEngineInterface::QueuedEngineInterface(size_t queue_size)
: QueuedEventSource(queue_size)
, _responder(CountedPtr<Responder>(new Responder())) // NULL responder
{
}


/** Set the Responder to send responses to commands with, once the commands
 * are preprocessed and ready to be executed (or not).
 *
 * Ownership of @a responder is taken.
 */
void
QueuedEngineInterface::set_responder(CountedPtr<Responder> responder)
{
	//cerr << "SET\n";
	_responder = responder;
}


void
QueuedEngineInterface::disable_responses()
{
	static CountedPtr<Responder> null_responder(new Responder());
	//cerr << "DISABLE\n";
	set_responder(null_responder);
}


/* *** EngineInterface implementation below here *** */


void
QueuedEngineInterface::register_client(ClientKey key, CountedPtr<ClientInterface> client)
{
	RegisterClientEvent* ev = new RegisterClientEvent(_responder, key, client);
	push(ev);
}


void
QueuedEngineInterface::unregister_client(ClientKey key)
{
	UnregisterClientEvent* ev = new UnregisterClientEvent(_responder, key);
	push(ev);
}



// Engine commands
void
QueuedEngineInterface::load_plugins()
{
	LoadPluginsEvent* ev = new LoadPluginsEvent(_responder);
	push(ev);

}


void
QueuedEngineInterface::activate()    
{
	ActivateEvent* ev = new ActivateEvent(_responder);
	push(ev);
}


void
QueuedEngineInterface::deactivate()  
{
	DeactivateEvent* ev = new DeactivateEvent(_responder);
	push(ev);
}


void
QueuedEngineInterface::quit()        
{
	_responder->respond_ok();
	om->quit();
}


		
// Object commands

void
QueuedEngineInterface::create_patch(const string& path,
                                    uint32_t      poly)
{
	CreatePatchEvent* ev = new CreatePatchEvent(_responder, path, poly);
	push(ev);

}


void QueuedEngineInterface::create_port(const string& path,
                                        const string& data_type,
                                        bool          direction)
{
	AddPortEvent* ev = new AddPortEvent(_responder, path, data_type, direction);
	push(ev);
}


void
QueuedEngineInterface::create_node(const string& path,
                                   const string& plugin_type,
                                   const string& plugin_uri,
                                   bool          polyphonic)
{
	// FIXME: ew
	
	Plugin* plugin = new Plugin();
	plugin->set_type(plugin_type);
	plugin->uri(plugin_uri);

	AddNodeEvent* ev = new AddNodeEvent(_responder,
		path, plugin, polyphonic);
	push(ev);
}


void
QueuedEngineInterface::rename(const string& old_path,
                              const string& new_name)
{
	RenameEvent* ev = new RenameEvent(_responder, old_path, new_name);
	push(ev);
}


void
QueuedEngineInterface::destroy(const string& path)
{
	DestroyEvent* ev = new DestroyEvent(_responder, this, path);
	push(ev);
}


void
QueuedEngineInterface::clear_patch(const string& patch_path)
{
}


void
QueuedEngineInterface::enable_patch(const string& patch_path)
{
	EnablePatchEvent* ev = new EnablePatchEvent(_responder, patch_path);
	push(ev);
}


void
QueuedEngineInterface::disable_patch(const string& patch_path)
{
	DisablePatchEvent* ev = new DisablePatchEvent(_responder, patch_path);
	push(ev);
}


void
QueuedEngineInterface::connect(const string& src_port_path,
                               const string& dst_port_path)
{
	ConnectionEvent* ev = new ConnectionEvent(_responder, src_port_path, dst_port_path);
	push(ev);

}


void
QueuedEngineInterface::disconnect(const string& src_port_path,
                                  const string& dst_port_path)
{
	DisconnectionEvent* ev = new DisconnectionEvent(_responder, src_port_path, dst_port_path);
	push(ev);
}


void
QueuedEngineInterface::disconnect_all(const string& node_path)
{
	DisconnectNodeEvent* ev = new DisconnectNodeEvent(_responder, node_path);
	push(ev);
}


void
QueuedEngineInterface::set_port_value(const string& port_path,
                                      float         value)
{
	SetPortValueEvent* ev = new SetPortValueEvent(_responder, port_path, value);
	om->event_queue()->push(ev);
}


void
QueuedEngineInterface::set_port_value(const string& port_path,
                                      uint32_t      voice,
                                      float         value)
{
	SetPortValueEvent* ev = new SetPortValueEvent(_responder, voice, port_path, value);
	om->event_queue()->push(ev);
}


void
QueuedEngineInterface::set_port_value_queued(const string& port_path,
                                             float         value)
{
	SetPortValueQueuedEvent* ev = new SetPortValueQueuedEvent(_responder, port_path, value);
	push(ev);
}


void
QueuedEngineInterface::set_program(const string& node_path,
                                   uint32_t      bank,
                                   uint32_t      program)
{
	push(new DSSIProgramEvent(_responder, node_path, bank, program));
}


void
QueuedEngineInterface::midi_learn(const string& node_path)
{
	MidiLearnEvent* ev = new MidiLearnEvent(_responder, node_path);
	push(ev);
}


void
QueuedEngineInterface::set_metadata(const string& path,
                                    const string& predicate,
                                    const string& value)
{
	SetMetadataEvent* ev = new SetMetadataEvent(_responder,
		path, predicate, value);
	
	push(ev);
}


// Requests //

void
QueuedEngineInterface::ping()
{
	PingQueuedEvent* ev = new PingQueuedEvent(_responder);
	push(ev);
}


void
QueuedEngineInterface::request_port_value(const string& port_path)
{
	RequestPortValueEvent* ev = new RequestPortValueEvent(_responder, port_path);
	push(ev);
}


void
QueuedEngineInterface::request_plugins()
{
	RequestPluginsEvent* ev = new RequestPluginsEvent(_responder);
	push(ev);
}


void
QueuedEngineInterface::request_all_objects()
{
	RequestAllObjectsEvent* ev = new RequestAllObjectsEvent(_responder);
	push(ev);
}


} // namespace Om


