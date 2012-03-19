/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include <string>

#include "raul/log.hpp"

#include "ingen/shared/URIs.hpp"

#include "ClientBroadcaster.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "EventSource.hpp"
#include "ServerInterfaceImpl.hpp"
#include "events.hpp"

#define LOG(s) s << "[ServerInterfaceImpl] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

ServerInterfaceImpl::ServerInterfaceImpl(Engine& engine)
	: EventSource()
	, _request_client(NULL)
	, _request_id(-1)
	, _engine(engine)
	, _in_bundle(false)
{
	start();
}


ServerInterfaceImpl::~ServerInterfaceImpl()
{
	stop();
}

SampleCount
ServerInterfaceImpl::now() const
{
	// Exactly one cycle latency (some could run ASAP if we get lucky, but not always, and a slight
	// constant latency is far better than jittery lower (average) latency
	if (_engine.driver())
		return _engine.driver()->frame_time() + _engine.driver()->block_length();
	else
		return 0;
}

void
ServerInterfaceImpl::set_response_id(int32_t id)
{
	if (!_request_client) {  // Kludge
		_request_client = _engine.broadcaster()->client(
			"http://drobilla.net/ns/ingen#internal");
	}
	_request_id = id;
}

/* *** ServerInterface implementation below here *** */

// Bundle commands

void
ServerInterfaceImpl::bundle_begin()
{
	_in_bundle = true;
}

void
ServerInterfaceImpl::bundle_end()
{
	_in_bundle = false;
}

// Object commands

void
ServerInterfaceImpl::put(const URI&                  uri,
                         const Resource::Properties& properties,
                         const Resource::Graph       ctx)
{
	push_queued(new Events::SetMetadata(_engine, _request_client, _request_id, now(), true, ctx, uri, properties));
}

void
ServerInterfaceImpl::delta(const URI&                  uri,
                           const Resource::Properties& remove,
                           const Resource::Properties& add)
{
	push_queued(new Events::SetMetadata(_engine, _request_client, _request_id, now(), false, Resource::DEFAULT, uri, add, remove));
}

void
ServerInterfaceImpl::move(const Path& old_path,
                          const Path& new_path)
{
	push_queued(new Events::Move(_engine, _request_client, _request_id, now(), old_path, new_path));
}

void
ServerInterfaceImpl::del(const URI& uri)
{
	if (uri == "ingen:engine") {
		if (_request_client) {
			_request_client->response(_request_id, SUCCESS);
		}
		_engine.quit();
	} else {
		push_queued(new Events::Delete(_engine, _request_client, _request_id, now(), uri));
	}
}

void
ServerInterfaceImpl::connect(const Path& src_port_path,
                             const Path& dst_port_path)
{
	push_queued(new Events::Connect(_engine, _request_client, _request_id, now(), src_port_path, dst_port_path));

}

void
ServerInterfaceImpl::disconnect(const URI& src,
                                const URI& dst)
{
	if (!Path::is_path(src) && !Path::is_path(dst)) {
		std::cerr << "Bad disconnect request " << src << " => " << dst << std::endl;
		return;
	}

	push_queued(new Events::Disconnect(_engine, _request_client, _request_id, now(),
	                                   Path(src.str()), Path(dst.str())));
}

void
ServerInterfaceImpl::disconnect_all(const Path& patch_path,
                                    const Path& path)
{
	push_queued(new Events::DisconnectAll(_engine, _request_client, _request_id, now(), patch_path, path));
}

void
ServerInterfaceImpl::set_property(const URI&  uri,
                                  const URI&  predicate,
                                  const Atom& value)
{
	if (uri == "ingen:engine" && predicate == "ingen:enabled"
	    && value.type() == _engine.world()->forge().Bool) {
		if (value.get_bool()) {
			_engine.activate();
			push_queued(new Events::Ping(_engine, _request_client, _request_id, now()));
		} else {
			push_queued(new Events::Deactivate(_engine, _request_client, _request_id, now()));
		}
	} else {
		Resource::Properties remove;
		remove.insert(make_pair(predicate, _engine.world()->uris()->wildcard));
		Resource::Properties add;
		add.insert(make_pair(predicate, value));
		push_queued(new Events::SetMetadata(
			            _engine, _request_client, _request_id, now(), false, Resource::DEFAULT,
			            uri, add, remove));
	}
}

// Requests //

void
ServerInterfaceImpl::get(const URI& uri)
{
	push_queued(new Events::Get(_engine, _request_client, _request_id, now(), uri));
}

} // namespace Server
} // namespace Ingen
