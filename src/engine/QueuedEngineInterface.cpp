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

#include "raul/log.hpp"
#include "QueuedEngineInterface.hpp"
#include "tuning.hpp"
#include "EventSource.hpp"
#include "events.hpp"
#include "Engine.hpp"
#include "Driver.hpp"

#define LOG(s) s << "[QueuedEngineInterface] "

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

QueuedEngineInterface::QueuedEngineInterface(Engine& engine, size_t queue_size)
	: EventSource(queue_size)
	, _request(new Request(this, NULL, 0))
	, _engine(engine)
	, _in_bundle(false)
{
}


SampleCount
QueuedEngineInterface::now() const
{
	// Exactly one cycle latency (some could run ASAP if we get lucky, but not always, and a slight
	// constant latency is far better than jittery lower (average) latency
	if (_engine.driver())
		return _engine.driver()->frame_time() + _engine.driver()->block_length();
	else
		return 0;
}


void
QueuedEngineInterface::set_next_response_id(int32_t id)
{
	if (_request)
		_request->set_id(id);
}


void
QueuedEngineInterface::disable_responses()
{
	_request->set_client(NULL);
	_request->set_id(0);
}


/* *** EngineInterface implementation below here *** */


void
QueuedEngineInterface::register_client(ClientInterface* client)
{
	push_queued(new Events::RegisterClient(_engine, _request, now(), client->uri(), client));
	if (!_request) {
		_request = SharedPtr<Request>(new Request(this, client, 1));
	} else {
		_request->set_id(1);
		_request->set_client(client);
	}
}


void
QueuedEngineInterface::unregister_client(const URI& uri)
{
	push_queued(new Events::UnregisterClient(_engine, _request, now(), uri));
	if (_request && _request->client() && _request->client()->uri() == uri) {
		_request->set_id(0);
		_request->set_client(NULL);
	}
}



// Engine commands
void
QueuedEngineInterface::load_plugins()
{
	push_queued(new Events::LoadPlugins(_engine, _request, now()));
}


void
QueuedEngineInterface::activate()
{
	static bool in_activate = false;
	if (!in_activate) {
		in_activate = true;
		_engine.activate();
	}
	EventSource::activate_source();
	push_queued(new Events::Ping(_engine, _request, now()));
	in_activate = false;
}


void
QueuedEngineInterface::deactivate()
{
	push_queued(new Events::Deactivate(_engine, _request, now()));
}


void
QueuedEngineInterface::quit()
{
	_request->respond_ok();
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
	bool meta = ResourceImpl::is_meta_uri(uri);
	URI  subject(meta ? (string("path:/") + uri.substr(6)) : uri.str());

	push_queued(new Events::SetMetadata(_engine, _request, now(), true, meta, subject, properties));
}


void
QueuedEngineInterface::delta(const URI&                          uri,
                             const Shared::Resource::Properties& remove,
                             const Shared::Resource::Properties& add)
{
	bool meta = ResourceImpl::is_meta_uri(uri);
	URI  subject(meta ? (string("path:/") + uri.substr(6)) : uri.str());

	push_queued(new Events::SetMetadata(_engine, _request, now(), false, meta, subject, add, remove));
}


void
QueuedEngineInterface::move(const Path& old_path,
                            const Path& new_path)
{
	push_queued(new Events::Move(_engine, _request, now(), old_path, new_path));
}


void
QueuedEngineInterface::del(const Path& path)
{
	push_queued(new Events::Delete(_engine, _request, now(), path));
}


void
QueuedEngineInterface::connect(const Path& src_port_path,
                               const Path& dst_port_path)
{
	push_queued(new Events::Connect(_engine, _request, now(), src_port_path, dst_port_path));

}


void
QueuedEngineInterface::disconnect(const Path& src_port_path,
                                  const Path& dst_port_path)
{
	push_queued(new Events::Disconnect(_engine, _request, now(), src_port_path, dst_port_path));
}


void
QueuedEngineInterface::disconnect_all(const Path& patch_path,
                                      const Path& path)
{
	push_queued(new Events::DisconnectAll(_engine, _request, now(), patch_path, path));
}


void
QueuedEngineInterface::set_property(const URI&  uri,
                                    const URI&  predicate,
                                    const Atom& value)
{
	size_t hash = uri.find("#");
	bool   meta = (hash != string::npos);
	Path path = meta ? (string("/") + path.chop_start("/")) : uri.str();
	Resource::Properties remove;
	remove.insert(make_pair(predicate, Shared::LV2URIMap::instance().wildcard));
	Resource::Properties add;
	add.insert(make_pair(predicate, value));
	push_queued(new Events::SetMetadata(_engine, _request, now(), false, meta, path, add, remove));
}

// Requests //

void
QueuedEngineInterface::ping()
{
	if (_engine.activated()) {
		push_queued(new Events::Ping(_engine, _request, now()));
	} else if (_request) {
		_request->respond_ok();
	}
}


void
QueuedEngineInterface::get(const URI& uri)
{
	push_queued(new Events::Get(_engine, _request, now(), uri));
}


void
QueuedEngineInterface::request_property(const URI& uri, const URI& key)
{
	size_t hash = uri.find("#");
	bool   meta = (hash != string::npos);
	const string path_str = string("/") + uri.chop_start("/");
	if (meta && Path::is_valid(path_str))
		push_queued(new Events::RequestMetadata(_engine, _request, now(), meta, path_str, key));
	else
		push_queued(new Events::RequestMetadata(_engine, _request, now(), meta, uri, key));
}


void
QueuedEngineInterface::request_plugins()
{
	push_queued(new Events::RequestPlugins(_engine, _request, now()));
}


} // namespace Ingen
