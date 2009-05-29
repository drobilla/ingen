/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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
#include <libsoup/soup.h>
#include "raul/AtomRDF.hpp"
#include "redlandmm/Model.hpp"
#include "module/World.hpp"
#include "HTTPEngineSender.hpp"


using namespace std;
using namespace Raul;

namespace Ingen {
using namespace Shared;
namespace Client {


HTTPEngineSender::HTTPEngineSender(const World* world, const URI& engine_url)
	: _world(*world->rdf_world)
	, _engine_url(engine_url)
	, _id(0)
	, _enabled(true)
{
	_session = soup_session_sync_new();
}


HTTPEngineSender::~HTTPEngineSender()
{
	soup_session_abort(_session);
}


void
HTTPEngineSender::attach(int32_t ping_id, bool block)
{
	/*SoupMessage *msg;
	msg = soup_message_new ("GET", _engine_url.c_str());
	int status = soup_session_send_message (_session, msg);
	cout << "STATUS: " << status << endl;
	cout << "RESPONSE: " << msg->response_body->data << endl;*/
}


/* *** EngineInterface implementation below here *** */


/** Register with the engine via HTTP.
 *
 * Note that this does not actually use 'key', since the engine creates
 * it's own key for HTTP clients (namely the incoming URL), for NAT
 * traversal.  It is a parameter to remain compatible with EngineInterface.
 */
void
HTTPEngineSender::register_client(ClientInterface* client)
{
}


void
HTTPEngineSender::unregister_client(const URI& uri)
{
}


// Engine commands
void
HTTPEngineSender::load_plugins()
{
}


void
HTTPEngineSender::activate()
{
}


void
HTTPEngineSender::deactivate()
{
}


void
HTTPEngineSender::quit()
{
}



// Object commands


void
HTTPEngineSender::message_callback(SoupSession* session, SoupMessage* msg, void* ptr)
{
	cerr << "HTTP CALLBACK" << endl;
}


void
HTTPEngineSender::put(const URI&                  uri,
	                  const Resource::Properties& properties)
{
	const string path     = (uri.substr(0, 6) == "path:/") ? uri.substr(6) : uri.str();
	const string full_uri = _engine_url.str() + "/" + path;

	Redland::Model model(_world);
	for (Resource::Properties::const_iterator i = properties.begin(); i != properties.end(); ++i)
		model.add_statement(
				Redland::Resource(_world, path),
				i->first.str(),
				AtomRDF::atom_to_node(_world, i->second));

	const string str = model.serialise_to_string();
	SoupMessage* msg = soup_message_new("PUT", full_uri.c_str());
	assert(msg);
	soup_message_set_request(msg, "application/x-turtle", SOUP_MEMORY_COPY, str.c_str(), str.length());
	soup_session_send_message(_session, msg);
}


void
HTTPEngineSender::move(const Path& old_path,
                       const Path& new_path)
{
}


void
HTTPEngineSender::del(const Path& path)
{
}


void
HTTPEngineSender::clear_patch(const Path& patch_path)
{
}


void
HTTPEngineSender::connect(const Path& src_port_path,
                          const Path& dst_port_path)
{
}


void
HTTPEngineSender::disconnect(const Path& src_port_path,
                             const Path& dst_port_path)
{
}


void
HTTPEngineSender::disconnect_all(const Path& parent_patch_path,
                                 const Path& path)
{
}


void
HTTPEngineSender::set_port_value(const Path& port_path,
                                 const Atom& value)
{
}


void
HTTPEngineSender::set_voice_value(const Path& port_path,
                                  uint32_t    voice,
                                  const Atom& value)
{
}


void
HTTPEngineSender::midi_learn(const Path& node_path)
{
}


void
HTTPEngineSender::set_property(const URI&  subject,
                               const URI&  predicate,
                               const Atom& value)
{
}



// Requests //

void
HTTPEngineSender::ping()
{
}


void
HTTPEngineSender::get(const URI& uri)
{
}


void
HTTPEngineSender::request_property(const URI& object_path, const URI& key)
{
}


void
HTTPEngineSender::request_plugins()
{
}


void
HTTPEngineSender::request_all_objects()
{
}



} // namespace Client
} // namespace Ingen


