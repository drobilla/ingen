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
#include "raul/AtomLiblo.hpp"
#include "raul/Path.hpp"
#include "OSCEngineSender.hpp"
#include "common/interface/Patch.hpp"
#include "common/interface/Port.hpp"
#include "common/interface/Plugin.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {


/** Note the sending port is implicitly set by liblo, lo_send by default sends
 * from the most recently created server, so create the OSC listener before
 * this to have it all happen on the same port.  Yeah, this is a big magic :/
 */
OSCEngineSender::OSCEngineSender(const URI& engine_url)
	: _engine_url(engine_url)
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
		cerr << "Aborting: Unable to connect to " << _engine_url << endl;
		exit(EXIT_FAILURE);
	}

	cout << "[OSCEngineSender] Attempting to contact engine at " << _engine_url << " ..." << endl;

	_id = ping_id;
	this->ping();
}

/* *** EngineInterface implementation below here *** */


/** Register with the engine via OSC.
 *
 * Note that this does not actually use 'client', since the engine creates
 * it's own key for OSC clients (namely the incoming URL), for NAT
 * traversal.  It is a parameter to remain compatible with EngineInterface.
 */
void
OSCEngineSender::register_client(Shared::ClientInterface* client)
{
	send("/ingen/register_client", "i", next_id(), LO_ARGS_END, LO_ARGS_END);
}


void
OSCEngineSender::unregister_client(const URI& uri)
{
	send("/ingen/unregister_client", "i", next_id(), LO_ARGS_END);
}


// Engine commands
void
OSCEngineSender::load_plugins()
{
	send("/ingen/load_plugins", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::activate()
{
	send("/ingen/activate", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::deactivate()
{
	send("/ingen/deactivate", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::quit()
{
	send("/ingen/quit", "i", next_id(), LO_ARGS_END);
}



// Object commands


void
OSCEngineSender::put(const Raul::URI&                    path,
                     const Shared::Resource::Properties& properties)
{
	typedef Shared::Resource::Properties::const_iterator iterator;
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, path.c_str());
	for (iterator i = properties.begin(); i != properties.end(); ++i) {
		lo_message_add_string(m, i->first.c_str());
		Raul::AtomLiblo::lo_message_add_atom(m, i->second);
	}
	send_message("/ingen/put", m);
}


void
OSCEngineSender::move(const Path& old_path,
                      const Path& new_path)
{
	send("/ingen/move", "iss",
		next_id(),
		old_path.c_str(),
		new_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::del(const Path& path)
{
	send("/ingen/delete", "is",
		next_id(),
		path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::clear_patch(const Path& patch_path)
{
	send("/ingen/clear_patch", "is",
		next_id(),
		patch_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::connect(const Path& src_port_path,
                         const Path& dst_port_path)
{
	send("/ingen/connect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::disconnect(const Path& src_port_path,
                            const Path& dst_port_path)
{
	send("/ingen/disconnect", "iss",
		next_id(),
		src_port_path.c_str(),
		dst_port_path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::disconnect_all(const Path& parent_patch_path,
                                const Path& path)
{
	send("/ingen/disconnect_all", "iss",
		next_id(),
		parent_patch_path.c_str(),
		path.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::set_port_value(const Path& port_path,
                                const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, port_path.c_str());
	if (value.type() == Atom::BLOB)
		lo_message_add_string(m, value.get_blob_type());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


void
OSCEngineSender::set_voice_value(const Path& port_path,
                                 uint32_t    voice,
                                 const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_int32(m, next_id());
	lo_message_add_string(m, port_path.c_str());
	lo_message_add_int32(m, voice);
	if (value.type() == Atom::BLOB)
		lo_message_add_string(m, value.get_blob_type());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


void
OSCEngineSender::midi_learn(const Path& node_path)
{
	send("/ingen/midi_learn", "is",
		next_id(),
		node_path.c_str(),
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
	send_message("/ingen/set_property", m);
}



// Requests //

void
OSCEngineSender::ping()
{
	send("/ingen/ping", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::request_object(const URI& uri)
{
	send("/ingen/request_object", "is",
		next_id(),
		uri.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::request_variable(const URI& object_path, const URI& key)
{
	send("/ingen/request_variable", "iss",
		next_id(),
		object_path.c_str(),
		key.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::request_property(const URI& object_path, const URI& key)
{
	send("/ingen/request_property", "iss",
		next_id(),
		object_path.c_str(),
		key.c_str(),
		LO_ARGS_END);
}


void
OSCEngineSender::request_plugins()
{
	send("/ingen/request_plugins", "i", next_id(), LO_ARGS_END);
}


void
OSCEngineSender::request_all_objects()
{
	send("/ingen/request_all_objects", "i", next_id(), LO_ARGS_END);
}


} // namespace Client
} // namespace Ingen


