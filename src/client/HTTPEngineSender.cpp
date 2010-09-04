/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#include <libsoup/soup.h>
#include "raul/log.hpp"
#include "raul/AtomRDF.hpp"
#include "redlandmm/Model.hpp"
#include "module/World.hpp"
#include "HTTPEngineSender.hpp"
#include "HTTPClientReceiver.hpp"

#define LOG(s) s << "[HTTPEngineSender] "

using namespace std;
using namespace Raul;

namespace Ingen {
using namespace Shared;
namespace Client {


HTTPEngineSender::HTTPEngineSender(World* world, const URI& engine_url)
	: _world(*world->rdf_world())
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
	LOG(debug) << "Attaching to " << _engine_url << endl;
	SoupMessage* msg = soup_message_new ("GET", _engine_url.c_str());
	HTTPClientReceiver::send(msg);
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
	/*SoupMessage* msg = soup_message_new("GET", (_engine_url.str() + "/stream").c_str());
	HTTPClientReceiver::send(msg);*/
}


void
HTTPEngineSender::unregister_client(const URI& uri)
{
}


// Engine commands
void
HTTPEngineSender::load_plugins()
{
	SoupMessage* msg = soup_message_new("GET", (_engine_url.str() + "/plugins").c_str());
	HTTPClientReceiver::send(msg);
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
				AtomRDF::atom_to_node(model, i->second));

	const string str = model.serialise_to_string();
	SoupMessage* msg = soup_message_new(SOUP_METHOD_PUT, full_uri.c_str());
	assert(msg);
	soup_message_set_request(msg, "application/x-turtle", SOUP_MEMORY_COPY, str.c_str(), str.length());
	soup_session_send_message(_session, msg);
}


void
HTTPEngineSender::delta(const Raul::URI&                    path,
	                    const Shared::Resource::Properties& remove,
	                    const Shared::Resource::Properties& add)
{
	warn << "FIXME: HTTP DELTA" << endl;
}


void
HTTPEngineSender::move(const Path& old_path,
                       const Path& new_path)
{
	SoupMessage* msg = soup_message_new(SOUP_METHOD_MOVE,
			(_engine_url.str() + old_path.str()).c_str());
	soup_message_headers_append(msg->request_headers, "Destination",
			(_engine_url.str()  + new_path.str()).c_str());
	soup_session_send_message(_session, msg);
}


void
HTTPEngineSender::del(const Path& uri)
{
	const string path     = (uri.substr(0, 6) == "path:/") ? uri.substr(6) : uri.str();
	const string full_uri = _engine_url.str() + "/" + path;
	SoupMessage* msg = soup_message_new(SOUP_METHOD_DELETE, full_uri.c_str());
	soup_session_send_message(_session, msg);
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
HTTPEngineSender::set_property(const URI&  subject,
                               const URI&  predicate,
                               const Atom& value)
{
	Resource::Properties prop;
	prop.insert(make_pair(predicate, value));
	put(subject, prop);
}



// Requests //

void
HTTPEngineSender::ping()
{
	LOG(debug) << "Ping " << _engine_url << endl;
	get(_engine_url);
}


void
HTTPEngineSender::get(const URI& uri)
{
	LOG(debug) << "Get " << _engine_url << endl;
	SoupMessage* msg = soup_message_new("GET", uri.c_str());
	HTTPClientReceiver::send(msg);
}


void
HTTPEngineSender::request_property(const URI& object_path, const URI& key)
{
	LOG(warn) << "TODO: request property" << endl;
}


} // namespace Client
} // namespace Ingen

