/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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
#include "HTTPEngineSender.hpp"

using namespace std;

namespace Ingen {
namespace Client {


HTTPEngineSender::HTTPEngineSender(const string& engine_url)
	: _engine_url(engine_url)
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
HTTPEngineSender::unregister_client(const string& uri)
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

bool
HTTPEngineSender::new_object(const Shared::GraphObject* object)
{
	return false;
}


void
HTTPEngineSender::new_patch(const string& path,
                            uint32_t      poly)
{
}


void
HTTPEngineSender::new_port(const string& path,
                           const string& type,
                           uint32_t      index,
                           bool          is_output)
{
	const string uri = _engine_url + "/patch" + path;
	cout << "HTTP " << uri << " NEW PORT: " << path << endl;
	SoupMessage* msg = soup_message_new("PUT", uri.c_str());
	string str = string("NEW PORT").append(path).append(type);
	soup_message_set_request(msg, "application/x-turtle",
			SOUP_MEMORY_COPY, str.c_str(), str.length());
	soup_session_send_message(_session, msg);
}


void
HTTPEngineSender::new_node(const string& path,
                           const string& plugin_uri)
{
}


/** Create a node using library name and plugin label (DEPRECATED).
 *
 * DO NOT USE THIS.
 */
void
HTTPEngineSender::new_node_deprecated(const string& path,
                                      const string& plugin_type,
                                      const string& library_name,
                                      const string& plugin_label)
{
}


void
HTTPEngineSender::rename(const string& old_path,
                         const string& new_name)
{
}


void
HTTPEngineSender::destroy(const string& path)
{
}


void
HTTPEngineSender::clear_patch(const string& patch_path)
{
}


void
HTTPEngineSender::connect(const string& src_port_path,
                          const string& dst_port_path)
{
}


void
HTTPEngineSender::disconnect(const string& src_port_path,
                             const string& dst_port_path)
{
}


void
HTTPEngineSender::disconnect_all(const string& parent_patch_path,
                                 const string& node_path)
{
}


void
HTTPEngineSender::set_port_value(const string&     port_path,
                                 const Raul::Atom& value)
{
}


void
HTTPEngineSender::set_voice_value(const string&     port_path,
                                  uint32_t          voice,
                                  const Raul::Atom& value)
{
}


void
HTTPEngineSender::set_program(const string& node_path,
                              uint32_t      bank,
                              uint32_t      program)
{
}


void
HTTPEngineSender::midi_learn(const string& node_path)
{
}


void
HTTPEngineSender::set_variable(const string&     obj_path,
                               const string&     predicate,
                               const Raul::Atom& value)
{
}


void
HTTPEngineSender::set_property(const string&    obj_path,
                               const string&     predicate,
                               const Raul::Atom& value)
{
}



// Requests //

void
HTTPEngineSender::ping()
{
}


void
HTTPEngineSender::request_plugin(const string& uri)
{
}


void
HTTPEngineSender::request_object(const string& path)
{
}


void
HTTPEngineSender::request_port_value(const string& port_path)
{
}


void
HTTPEngineSender::request_variable(const string& object_path, const string& key)
{
}


void
HTTPEngineSender::request_property(const string& object_path, const string& key)
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


