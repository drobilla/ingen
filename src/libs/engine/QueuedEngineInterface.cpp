/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "QueuedEngineInterface.h"
#include "QueuedEventSource.h"
#include "events.h"
#include "util/Queue.h"
#include "Engine.h"
#include "AudioDriver.h"

namespace Ingen {

QueuedEngineInterface::QueuedEngineInterface(CountedPtr<Engine> engine, size_t queued_size, size_t stamped_size)
: QueuedEventSource(queued_size, stamped_size)
, _responder(CountedPtr<Responder>(new Responder())) // NULL responder
, _engine(engine)
{
}


SampleCount
QueuedEngineInterface::now() const
{
	// Exactly one cycle latency (some could run ASAP if we get lucky, but not always, and a slight
	// constant latency is far better than jittery lower (average) latency
	return _engine->audio_driver()->frame_time() + _engine->audio_driver()->buffer_size();
}


/** Set the Responder to send responses to commands with, once the commands
 * are preprocessed and ready to be executed (or not).
 *
 * Ownership of @a responder is taken.
 */
void
QueuedEngineInterface::set_responder(CountedPtr<Responder> responder)
{
	_responder = responder;
}


void
QueuedEngineInterface::set_next_response_id(int32_t id)
{
	if (_responder)
		_responder->set_id(id);
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
	push_queued(new RegisterClientEvent(*_engine.get(), _responder, now(), key, client));
}


void
QueuedEngineInterface::unregister_client(ClientKey key)
{
	push_queued(new UnregisterClientEvent(*_engine.get(), _responder, now(), key));
}



// Engine commands
void
QueuedEngineInterface::load_plugins()
{
	push_queued(new LoadPluginsEvent(*_engine.get(), _responder, now()));

}


void
QueuedEngineInterface::activate()    
{
	push_queued(new ActivateEvent(*_engine.get(), _responder, now()));
}


void
QueuedEngineInterface::deactivate()  
{
	push_queued(new DeactivateEvent(*_engine.get(), _responder, now()));
}


void
QueuedEngineInterface::quit()        
{
	_responder->respond_ok();
	_engine->quit();
}


		
// Object commands

void
QueuedEngineInterface::create_patch(const string& path,
                                    uint32_t      poly)
{
	push_queued(new CreatePatchEvent(*_engine.get(), _responder, now(), path, poly));

}


void QueuedEngineInterface::create_port(const string& path,
                                        const string& data_type,
                                        bool          direction)
{
	push_queued(new AddPortEvent(*_engine.get(), _responder, now(), path, data_type, direction));
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

	push_queued(new AddNodeEvent(*_engine.get(), _responder, now(), path, plugin, polyphonic));
}


void
QueuedEngineInterface::create_node(const string& path,
                                   const string& plugin_type,
                                   const string& plugin_lib,
                                   const string& plugin_label,
                                   bool          polyphonic)
{
	// FIXME: ew
	
	Plugin* plugin = new Plugin();
	plugin->set_type(plugin_type);
	plugin->lib_name(plugin_lib);
	plugin->plug_label(plugin_label);

	push_queued(new AddNodeEvent(*_engine.get(), _responder, now(), path, plugin, polyphonic));
}

void
QueuedEngineInterface::rename(const string& old_path,
                              const string& new_name)
{
	push_queued(new RenameEvent(*_engine.get(), _responder, now(), old_path, new_name));
}


void
QueuedEngineInterface::destroy(const string& path)
{
	push_queued(new DestroyEvent(*_engine.get(), _responder, now(), this, path));
}


void
QueuedEngineInterface::clear_patch(const string& patch_path)
{
	push_queued(new ClearPatchEvent(*_engine.get(), _responder, now(), patch_path));
}


void
QueuedEngineInterface::enable_patch(const string& patch_path)
{
	push_queued(new EnablePatchEvent(*_engine.get(), _responder, now(), patch_path));
}


void
QueuedEngineInterface::disable_patch(const string& patch_path)
{
	push_queued(new DisablePatchEvent(*_engine.get(), _responder, now(), patch_path));
}


void
QueuedEngineInterface::connect(const string& src_port_path,
                               const string& dst_port_path)
{
	push_queued(new ConnectionEvent(*_engine.get(), _responder, now(), src_port_path, dst_port_path));

}


void
QueuedEngineInterface::disconnect(const string& src_port_path,
                                  const string& dst_port_path)
{
	push_queued(new DisconnectionEvent(*_engine.get(), _responder, now(), src_port_path, dst_port_path));
}


void
QueuedEngineInterface::disconnect_all(const string& node_path)
{
	push_queued(new DisconnectNodeEvent(*_engine.get(), _responder, now(), node_path));
}


void
QueuedEngineInterface::set_port_value(const string& port_path,
                                      float         value)
{
	push_stamped(new SetPortValueEvent(*_engine.get(), _responder, now(), port_path, value));
}


void
QueuedEngineInterface::set_port_value(const string& port_path,
                                      uint32_t      voice,
                                      float         value)
{
	push_stamped(new SetPortValueEvent(*_engine.get(), _responder, now(), voice, port_path, value));
}


void
QueuedEngineInterface::set_port_value_queued(const string& port_path,
                                             float         value)
{
	push_queued(new SetPortValueQueuedEvent(*_engine.get(), _responder, now(), port_path, value));
}


void
QueuedEngineInterface::set_program(const string& node_path,
                                   uint32_t      bank,
                                   uint32_t      program)
{
	push_queued(new DSSIProgramEvent(*_engine.get(), _responder, now(), node_path, bank, program));
}


void
QueuedEngineInterface::midi_learn(const string& node_path)
{
	push_queued(new MidiLearnEvent(*_engine.get(), _responder, now(), node_path));
}


void
QueuedEngineInterface::set_metadata(const string& path,
                                    const string& predicate,
                                    const string& value)
{
	push_queued(new SetMetadataEvent(*_engine.get(), _responder, now(), path, predicate, value));
}


// Requests //

void
QueuedEngineInterface::ping()
{
	if (_engine->activated()) {
		push_queued(new PingQueuedEvent(*_engine.get(), _responder, now()));
	} else if (_responder) {
		_responder->respond_ok();
	}
}


void
QueuedEngineInterface::request_port_value(const string& port_path)
{
	push_queued(new RequestPortValueEvent(*_engine.get(), _responder, now(), port_path));
}


void
QueuedEngineInterface::request_plugins()
{
	push_queued(new RequestPluginsEvent(*_engine.get(), _responder, now()));
}


void
QueuedEngineInterface::request_all_objects()
{
	push_queued(new RequestAllObjectsEvent(*_engine.get(), _responder, now()));
}


} // namespace Ingen


