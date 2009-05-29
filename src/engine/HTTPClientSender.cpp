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

#include <string>
#include <libsoup/soup.h>
#include "raul/Atom.hpp"
#include "raul/AtomRDF.hpp"
#include "serialisation/Serialiser.hpp"
#include "module/World.hpp"
#include "HTTPClientSender.hpp"
#include "Engine.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

void
HTTPClientSender::response_ok(int32_t id)
{
}


void
HTTPClientSender::response_error(int32_t id, const std::string& msg)
{
	cout << "HTTP ERROR " << id << ": " << msg << endl;
}


void
HTTPClientSender::error(const std::string& msg)
{
	//send("/ingen/error", "s", msg.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::put(const URI&                  uri,
                      const Resource::Properties& properties)
{
	const string path     = (uri.substr(0, 6) == "path:/") ? uri.substr(6) : uri.str();
	const string full_uri = _url + "/" + path;

	Redland::Model model(*_engine.world()->rdf_world);
	for (Resource::Properties::const_iterator i = properties.begin(); i != properties.end(); ++i)
		model.add_statement(
				Redland::Resource(*_engine.world()->rdf_world, path),
				i->first.str(),
				AtomRDF::atom_to_node(*_engine.world()->rdf_world, i->second));

	const string str = model.serialise_to_string();
	send_chunk(str);
}


void
HTTPClientSender::del(const Path& path)
{
	assert(!path.is_root());
	send_chunk(string("<").append(path.str()).append("> a <http://www.w3.org/2002/07/owl#Nothing> ."));
}


void
HTTPClientSender::clear_patch(const Path& patch_path)
{
	send_chunk(string("<").append(patch_path.str()).append("> ingen:empty true ."));
}


void
HTTPClientSender::connect(const Path& src_path, const Path& dst_path)
{
	const string msg = string(
			"@prefix rdf:       <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
			"@prefix ingen:     <http://drobilla.net/ns/ingen#> .\n").append(
			"<> ingen:connection [\n"
			"\tingen:destination <").append(dst_path.str()).append("> ;\n"
			"\tingen:source <").append(src_path.str()).append(">\n] .\n");
	send_chunk(msg);
}


void
HTTPClientSender::disconnect(const Path& src_path, const Path& dst_path)
{
	//send("/ingen/disconnection", "ss", src_path.c_str(), dst_path.c_str(), LO_ARGS_END);
}


void
HTTPClientSender::set_property(const URI& subject, const URI& key, const Atom& value)
{
	Redland::Node node = AtomRDF::atom_to_node(*_engine.world()->rdf_world, value);
	const string msg = string(
			"@prefix rdf:       <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
			"@prefix ingen:     <http://drobilla.net/ns/ingen#> .\n"
			"@prefix ingenuity: <http://drobilla.net/ns/ingenuity#> .\n").append(
			subject.str()).append("> ingen:property [\n"
			"rdf:predicate ").append(key.str()).append(" ;\n"
			"rdf:value     ").append(node.to_string()).append("\n] .\n");
	send_chunk(msg);
}


void
HTTPClientSender::set_port_value(const Path& port_path, const Atom& value)
{
	Redland::Node node = AtomRDF::atom_to_node(*_engine.world()->rdf_world, value);
	const string msg = string(
			"@prefix ingen: <http://drobilla.net/ns/ingen#> .\n\n<").append(
			port_path.str()).append("> ingen:value ").append(node.to_string()).append(" .\n");
	send_chunk(msg);
}


void
HTTPClientSender::set_voice_value(const Path& port_path, uint32_t voice, const Atom& value)
{
	/*lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);*/
}


void
HTTPClientSender::activity(const Path& path)
{
	const string msg = string(
			"@prefix ingen: <http://drobilla.net/ns/ingen#> .\n\n<").append(
			path.str()).append("> ingen:activity true .\n");
	send_chunk(msg);
}

#if 0
static void null_deleter(const GraphObject*) {}

bool
HTTPClientSender::new_object(const GraphObject* object)
{
	SharedPtr<Serialisation::Serialiser> serialiser = _engine.world()->serialiser;
	serialiser->start_to_string("/", "");
	// FIXME: kludge
	// FIXME: engine boost dependency?
	boost::shared_ptr<GraphObject> obj((GraphObject*)object, null_deleter);
	serialiser->serialise(obj);
	string str = serialiser->finish();
	send_chunk(str);
	return true;
}
#endif


void
HTTPClientSender::move(const Path& old_path, const Path& new_path)
{
	string msg = string(
			"@prefix rdf:       <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
			"@prefix ingen:     <http://drobilla.net/ns/ingen#> .\n\n<").append(
			old_path.str()).append("> rdf:subject <").append(new_path.str()).append("> .\n");
	send_chunk(msg);
}


} // namespace Ingen
