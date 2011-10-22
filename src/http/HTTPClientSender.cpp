/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#include "ingen/serialisation/Serialiser.hpp"
#include "ingen/shared/World.hpp"
#include "raul/Atom.hpp"
#include "raul/AtomRDF.hpp"
#include "raul/log.hpp"

#include "../server/Engine.hpp"

#include "HTTPClientSender.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

void
HTTPClientSender::response_ok(int32_t id)
{
}

void
HTTPClientSender::response_error(int32_t id, const std::string& msg)
{
	warn << "HTTP Error " << id << " (" << msg << ")" << endl;
}

void
HTTPClientSender::error(const std::string& msg)
{
	warn << "HTTP send error " << msg << endl;
}

void
HTTPClientSender::put(const URI&                  uri,
                      const Resource::Properties& properties,
                      Resource::Graph             ctx)
{
	const std::string request_uri = (Raul::Path::is_path(uri))
		? _url + "/patch" + uri.substr(uri.find("/"))
		: uri.str();


	Sord::Model model(*_engine.world()->rdf_world());
	for (Resource::Properties::const_iterator i = properties.begin();
	     i != properties.end(); ++i)
		model.add_statement(
				Sord::URI(*_engine.world()->rdf_world(), request_uri),
				AtomRDF::atom_to_node(model, i->first.str()),
				AtomRDF::atom_to_node(model, i->second));

	const string str = model.write_to_string("", SERD_TURTLE);
	send_chunk(str);
}

void
HTTPClientSender::delta(const URI&                  uri,
                        const Resource::Properties& remove,
                        const Resource::Properties& add)
{
}

void
HTTPClientSender::del(const URI& uri)
{
	send_chunk(string("<").append(uri.str()).append("> a <http://www.w3.org/2002/07/owl#Nothing> ."));
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
HTTPClientSender::disconnect(const URI& src,
                             const URI& dst)
{
}

void
HTTPClientSender::disconnect_all(const Raul::Path& parent_patch_path,
                                 const Raul::Path& path)
{
}

void
HTTPClientSender::set_property(const URI& subject, const URI& key, const Atom& value)
{
#if 0
	Sord::Node node = AtomRDF::atom_to_node(*_engine.world()->rdf_world(), value);
	const string msg = string(
			"@prefix rdf:     <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
			"@prefix ingen:   <http://drobilla.net/ns/ingen#> .\n"
			"@prefix ingenui: <http://drobilla.net/ns/ingenuity#> .\n").append(
			subject.str()).append("> ingen:property [\n"
			"rdf:predicate ").append(key.str()).append(" ;\n"
			"rdf:value     ").append(node.to_string()).append("\n] .\n");
	send_chunk(msg);
#endif
}

void
HTTPClientSender::activity(const Path& path, const Raul::Atom& value)
{
	if (value.type() == Atom::BOOL) {
		const string msg = string(
			"@prefix ingen: <http://drobilla.net/ns/ingen#> .\n\n<").append(
				path.str()).append("> ingen:activity true .\n");
		send_chunk(msg);
	} else if (value.type() == Atom::FLOAT) {
		const string msg = string(
			"@prefix ingen: <http://drobilla.net/ns/ingen#> .\n\n<").append(
				path.str()).append("> ingen:activity ").append(
					value.get_bool() ? "true" : "false").append(" .\n");
		send_chunk(msg);
	} else {
		warn << "Unknown activity type at " << path << endl;
	}
}

void
HTTPClientSender::move(const Path& old_path, const Path& new_path)
{
	string msg = string(
			"@prefix rdf:       <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
			"@prefix ingen:     <http://drobilla.net/ns/ingen#> .\n\n<").append(
			old_path.str()).append("> rdf:subject <").append(new_path.str()).append("> .\n");
	send_chunk(msg);
}

} // namespace Server
} // namespace Ingen