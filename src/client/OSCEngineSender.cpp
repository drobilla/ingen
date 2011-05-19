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

#include "raul/log.hpp"
#include "raul/AtomLiblo.hpp"
#include "raul/Path.hpp"

#include "ingen/Patch.hpp"
#include "ingen/Port.hpp"
#include "ingen/Plugin.hpp"

#include "OSCEngineSender.hpp"

#define LOG(s) s << "[OSCEngineSender] "

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

/** Note the sending port is implicitly set by liblo, lo_send by default sends
 * from the most recently created server, so create the OSC listener before
 * this to have it all happen on the same port.  Yeah, this is a big magic :/
 */
OSCEngineSender::OSCEngineSender(const URI& engine_url,
                                 size_t     max_packet_size)
	: Shared::OSCSender(max_packet_size)
	, _engine_url(engine_url)
	, _id(0)
{
	_address = lo_address_new_from_url(engine_url.c_str());
}

OSCEngineSender::~OSCEngineSender()
{
	lo_address_free(_address);
}

/** Attempt to connect to the engine (by pinging it).
 *
 * This doesn't register a client (or otherwise affect the client/engine state).
 * To check for success wait for the ping response with id @a ping_id (using the
 * relevant OSCClientReceiver).
 *
 * Passing a client_port of 0 will automatically choose a free port.  If the
 * @a block parameter is true, this function will not return until a connection
 * has successfully been made.
 */
void
OSCEngineSender::attach(int32_t ping_id, bool block)
{
	if (!_address)
		_address = lo_address_new_from_url(_engine_url.c_str());

	if (_address == NULL) {
		LOG(error) << "Unable to connect to " << _engine_url << endl;
		exit(EXIT_FAILURE);
	}

	LOG(info) << "Attempting to contact engine at " << _engine_url << " ..." << endl;

	_id = ping_id;
	this->ping();
}

/* *** ServerInterface implementation below here *** */

/** Register with the engine via OSC.
 *
 * Note that this does not actually use 'client', since the engine creates
 * it's own key for OSC clients (namely the incoming URL), for NAT
 * traversal.  It is a parameter to remain compatible with ServerInterface.
 */
void
OSCEngineSender::register_client(ClientInterface* client)
{
	send("/register_client", "i", next_id(), LO_ARGS_END);
}

void
OSCEngineSender::unregister_client(const URI& uri)
{
	send("/unregister_client", "i", next_id(), LO_ARGS_END);
}

// Object commands

void
OSCEngineSender::put(const Raul::URI&            path,
                     const Resource::Properties& properties,
                     Resource::Graph             ctx)
{
	typedef Resource::Properties::const_iterator iterator;
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, Resource::graph_to_uri(ctx).c_str());
	for (iterator i = properties.begin(); i != properties.end(); ++i) {
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
	}
	send_message("/put", m);
}

void
OSCEngineSender::delta(const Raul::URI&            path,
                       const Resource::Properties& remove,
                       const Resource::Properties& add)
{
	typedef Resource::Properties::const_iterator iterator;

	const bool bundle = !_bundle;
	if (bundle)
		bundle_begin();

	const int32_t id = next_id();
	send("/delta_begin", "is", id, path.c_str(), LO_ARGS_END);

	for (iterator i = remove.begin(); i != remove.end(); ++i) {
		lo_message m = lo_message_new();
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
		send_message("/delta_remove", m);
	}

	for (iterator i = add.begin(); i != add.end(); ++i) {
		lo_message m = lo_message_new();
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
		send_message("/delta_add", m);
	}

	send("/delta_end", "i", id, LO_ARGS_END);

	if (bundle)
		bundle_end();
}

void
OSCEngineSender::move(const Path& old_path,
                      const Path& new_path)
{
	send("/move", "iss",
		next_id(),
		old_path.c_str(),
		new_path.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::del(const URI& uri)
{
	send("/delete", "is",
		next_id(),
		uri.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::connect(const Path& src_port_path,
                         const Path& dst_port_path)
{
	send("/connect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::disconnect(const URI& src,
                            const URI& dst)
{
	send("/disconnect", "iss",
		next_id(),
		src.c_str(),
		dst.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::disconnect_all(const Path& parent_patch_path,
                                const Path& path)
{
	send("/disconnect_all", "iss",
		next_id(),
		parent_patch_path.c_str(),
		path.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::set_property(const URI&  subject,
                              const URI&  predicate,
                              const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, subject.c_str());
	lo_message_add_string(m, predicate.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/set_property", m);
}

// Requests //

void
OSCEngineSender::ping()
{
	send("/ping", "i", next_id(), LO_ARGS_END);
}

void
OSCEngineSender::get(const URI& uri)
{
	send("/get", "is",
		next_id(),
		uri.c_str(),
		LO_ARGS_END);
}

void
OSCEngineSender::request_property(const URI& object_path, const URI& key)
{
	send("/request_property", "iss",
		next_id(),
		object_path.c_str(),
		key.c_str(),
		LO_ARGS_END);
}

} // namespace Client
} // namespace Ingen

