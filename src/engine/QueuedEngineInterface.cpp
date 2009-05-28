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

#include <iostream>
#include "QueuedEngineInterface.hpp"
#include "tuning.hpp"
#include "QueuedEventSource.hpp"
#include "events.hpp"
#include "Engine.hpp"
#include "AudioDriver.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

QueuedEngineInterface::QueuedEngineInterface(Engine& engine, size_t queue_size)
	: QueuedEventSource(queue_size)
	, _responder(new Responder(NULL, 0))
	, _engine(engine)
	, _in_bundle(false)
{
}


SampleCount
QueuedEngineInterface::now() const
{
	// Exactly one cycle latency (some could run ASAP if we get lucky, but not always, and a slight
	// constant latency is far better than jittery lower (average) latency
	if (_engine.audio_driver())
		return _engine.audio_driver()->frame_time() + _engine.audio_driver()->buffer_size();
	else
		return 0;
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
	_responder->set_client(NULL);
	_responder->set_id(0);
}


/* *** EngineInterface implementation below here *** */


void
QueuedEngineInterface::register_client(ClientInterface* client)
{
	push_queued(new RegisterClientEvent(_engine, _responder, now(), client->uri(), client));
	if (!_responder) {
		_responder = SharedPtr<Responder>(new Responder(client, 1));
	} else {
		_responder->set_id(1);
		_responder->set_client(client);
	}
}


void
QueuedEngineInterface::unregister_client(const URI& uri)
{
	push_queued(new UnregisterClientEvent(_engine, _responder, now(), uri));
	if (_responder && _responder->client() && _responder->client()->uri() == uri) {
		_responder->set_id(0);
		_responder->set_client(NULL);
	}
}



// Engine commands
void
QueuedEngineInterface::load_plugins()
{
	push_queued(new LoadPluginsEvent(_engine, _responder, now(), this));
}


void
QueuedEngineInterface::activate()
{
	static bool in_activate = false;
	if (!in_activate) {
		in_activate = true;
		_engine.activate(1);
	}
	QueuedEventSource::activate_source();
	push_queued(new PingQueuedEvent(_engine, _responder, now()));
	in_activate = false;
}


void
QueuedEngineInterface::deactivate()
{
	push_queued(new DeactivateEvent(_engine, _responder, now()));
}


void
QueuedEngineInterface::quit()
{
	_responder->respond_ok();
	_engine.quit();
}


// Bundle commands

void
QueuedEngineInterface::bundle_begin()
{
	_in_bundle = true;
}


void
QueuedEngineInterface::bundle_end()
{
	_in_bundle = false;
}


// Object commands


void
QueuedEngineInterface::put(const URI&                  uri,
                           const Resource::Properties& properties)
{
	cerr << "PUT " << uri << endl;
	size_t hash = uri.find("#");
	bool   meta = (hash != string::npos);
	Path   path(meta ? (string("/") + uri.chop_start("#")) : uri.str());

	typedef Resource::Properties::const_iterator iterator;
	cerr << "ENGINE PUT " << path << " (" << path << ") {" << endl;
	for (iterator i = properties.begin(); i != properties.end(); ++i)
		cerr << "\t" << i->first << " = " << i->second << " :: " << i->second.type() << endl;
	cerr << "}" << endl;

	push_queued(new SetMetadataEvent(_engine, _responder, now(), this, meta, path, properties));
}


void
QueuedEngineInterface::move(const Path& old_path,
                            const Path& new_path)
{
	push_queued(new MoveEvent(_engine, _responder, now(), old_path, new_path));
}


void
QueuedEngineInterface::del(const Path& path)
{
	push_queued(new DeleteEvent(_engine, _responder, now(), this, path));
}


void
QueuedEngineInterface::clear_patch(const Path& patch_path)
{
	push_queued(new ClearPatchEvent(_engine, _responder, now(), this, patch_path));
}


void
QueuedEngineInterface::connect(const Path& src_port_path,
                               const Path& dst_port_path)
{
	push_queued(new ConnectionEvent(_engine, _responder, now(), src_port_path, dst_port_path));

}


void
QueuedEngineInterface::disconnect(const Path& src_port_path,
                                  const Path& dst_port_path)
{
	push_queued(new DisconnectionEvent(_engine, _responder, now(), src_port_path, dst_port_path));
}


void
QueuedEngineInterface::disconnect_all(const Path& patch_path,
                                      const Path& path)
{
	push_queued(new DisconnectAllEvent(_engine, _responder, now(), patch_path, path));
}


void
QueuedEngineInterface::set_port_value(const Path&       port_path,
                                      const Raul::Atom& value)
{
	push_queued(new SetPortValueEvent(_engine, _responder, true, now(), port_path, value));
}


void
QueuedEngineInterface::set_voice_value(const Path&       port_path,
                                       uint32_t          voice,
                                       const Raul::Atom& value)
{
	push_queued(new SetPortValueEvent(_engine, _responder, true, now(), voice, port_path, value));
}


void
QueuedEngineInterface::midi_learn(const Path& node_path)
{
	push_queued(new MidiLearnEvent(_engine, _responder, now(), node_path));
}


void
QueuedEngineInterface::set_property(const URI&  uri,
                                    const URI&  predicate,
                                    const Atom& value)
{
	size_t hash = uri.find("#");
	bool   meta = (hash != string::npos);
	Path path = meta ? (string("/") + path.chop_start("/")) : uri.str();
	Resource::Properties properties;
	properties.insert(make_pair(predicate, value));
	push_queued(new SetMetadataEvent(_engine, _responder, now(), this, meta, path, properties));
}

// Requests //

void
QueuedEngineInterface::ping()
{
	if (_engine.activated()) {
		push_queued(new PingQueuedEvent(_engine, _responder, now()));
	} else if (_responder) {
		_responder->respond_ok();
	}
}


void
QueuedEngineInterface::request_object(const URI& uri)
{
	push_queued(new RequestObjectEvent(_engine, _responder, now(), uri));
}


void
QueuedEngineInterface::request_variable(const URI& object_path, const URI& key)
{
	push_queued(new RequestMetadataEvent(_engine, _responder, now(), false, object_path, key));
}


void
QueuedEngineInterface::request_property(const URI& object_path, const URI& key)
{
	push_queued(new RequestMetadataEvent(_engine, _responder, now(), true, object_path, key));
}


void
QueuedEngineInterface::request_plugins()
{
	push_queued(new RequestPluginsEvent(_engine, _responder, now()));
}


void
QueuedEngineInterface::request_all_objects()
{
	push_queued(new RequestAllObjectsEvent(_engine, _responder, now()));
}


} // namespace Ingen


extern "C" {

Ingen::QueuedEngineInterface*
new_queued_interface(Ingen::Engine& engine)
{
	return new Ingen::QueuedEngineInterface(engine, Ingen::event_queue_size);
}

} // extern "C"

