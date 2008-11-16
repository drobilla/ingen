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

#include <string>
#include "raul/Atom.hpp"
#include "HTTPClientSender.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

void
HTTPClientSender::response_ok(int32_t id)
{
	cout << "HTTP OK" << endl;
}


void
HTTPClientSender::response_error(int32_t id, const std::string& msg)
{
	cout << "HTTP ERROR" << endl;
}


void
HTTPClientSender::error(const std::string& msg)
{
	//send("/ingen/error", "s", msg.c_str(), LO_ARGS_END);
}


void HTTPClientSender::new_node(const std::string& node_path,
                                const std::string& plugin_uri)
{
	//send("/ingen/new_node", "ss", node_path.c_str(), plugin_uri.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::new_port(const std::string& path,
                           const std::string& type,
                           uint32_t           index,
                           bool               is_output)
{
	//send("/ingen/new_port", "sisi", path.c_str(), index, type.c_str(), is_output, LO_ARGS_END);
}


void
HTTPClientSender::destroy(const std::string& path)
{
	assert(path != "/");
	
	//send("/ingen/destroyed", "s", path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::patch_cleared(const std::string& patch_path)
{
	//send("/ingen/patch_cleared", "s", patch_path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::connect(const std::string& src_path, const std::string& dst_path)
{
	//send("/ingen/new_connection", "ss", src_path.c_str(), dst_path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::disconnect(const std::string& src_path, const std::string& dst_path)
{
	//send("/ingen/disconnection", "ss", src_path.c_str(), dst_path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::set_variable(const std::string& path, const std::string& key, const Atom& value)
{
	/*lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, key.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_variable", m);*/
}


void
HTTPClientSender::set_property(const std::string& path, const std::string& key, const Atom& value)
{
	/*lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, key.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_property", m);*/
}


void
HTTPClientSender::set_port_value(const std::string& port_path, const Raul::Atom& value)
{
	/*lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);*/
}


void
HTTPClientSender::set_voice_value(const std::string& port_path, uint32_t voice, const Raul::Atom& value)
{
	/*lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	Raul::AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);*/
}


void
HTTPClientSender::activity(const std::string& path)
{
	//lo_send(_address, "/ingen/activity", "s", port_path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::new_plugin(const std::string& uri,
                             const std::string& type_uri,
                             const std::string& symbol,
                             const std::string& name)
{
	/*lo_message m = lo_message_new();
	lo_message_add_string(m, uri.c_str());
	lo_message_add_string(m, type_uri.c_str());
	lo_message_add_string(m, symbol.c_str());
	lo_message_add_string(m, name.c_str());
	send_message("/ingen/plugin", m);*/
}


void
HTTPClientSender::new_patch(const std::string& path, uint32_t poly)
{
	send_chunk(string("<").append(path).append("> a ingen:Patch"));
}


void
HTTPClientSender::object_renamed(const std::string& old_path, const std::string& new_path)
{
	//send("/ingen/object_renamed", "ss", old_path.c_str(), new_path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::program_add(const std::string& node_path, uint32_t bank, uint32_t program, const std::string& name)
{
	/*send("/ingen/program_add", "siis", 
		node_path.c_str(), bank, program, name.c_str(), LO_ARGS_END);*/
}


void
HTTPClientSender::program_remove(const std::string& node_path, uint32_t bank, uint32_t program)
{
	/*send("/ingen/program_remove", "sii", 
		node_path.c_str(), bank, program, LO_ARGS_END);*/
}


} // namespace Ingen
