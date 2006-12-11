/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include "Loader.h"
#include "RDFQuery.h"
#include "ModelEngineInterface.h"

namespace Ingen {
namespace Client {


Loader::Loader(SharedPtr<ModelEngineInterface> engine, SharedPtr<Namespaces> namespaces)
	: _engine(engine)
	, _namespaces(namespaces)
{
	if (!_namespaces)
		_namespaces = SharedPtr<Namespaces>(new Namespaces());

	// FIXME: hack
	_namespaces->add("ingen", "http://codeson.net/ns/ingen#");
	_namespaces->add("ingenuity", "http://codeson.net/ns/ingenuity#");
}


/** Load (create) all objects from an RDF into the engine.
 *
 * @param filename Filename to load objects from.
 * @param parent Path of parent under which to load objects.
 * @return whether or not load was successful.
 */
void
Loader::load(const Glib::ustring& filename,
             const Path& parent)
{
	std::map<Glib::ustring, bool> created;


	/* Load nodes */
	
	RDFQuery query(Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?floatkey ?floatval FROM <") + filename + "> WHERE {\n"
		"?patch ingen:node   ?node .\n"
		"?node  ingen:name   ?name ;\n"
		"       ingen:plugin ?plugin ;\n"
		"OPTIONAL { ?node ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"}");

	RDFQuery::Results nodes = query.run(filename);

	for (RDFQuery::Results::iterator i = nodes.begin(); i != nodes.end(); ++i) {
		const Glib::ustring& name   = (*i)["name"];
		const Glib::ustring& plugin = (*i)["plugin"];

		if (created.find(name) == created.end()) {
			_engine->create_node(parent.base() + name, plugin, false);
			created[name] = true;
		}

		Glib::ustring floatkey = _namespaces->qualify((*i)["floatkey"]);
		Glib::ustring floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(parent.base() + name, floatkey, Atom(val));
		}
	}
	
	created.clear();


	/* Load patch ports */
	
	query = RDFQuery(Glib::ustring(
		"SELECT DISTINCT ?port ?type ?name ?datatype ?floatkey ?floatval FROM <") + filename + "> WHERE {\n"
		"?patch ingen:port     ?port .\n"
		"?port  a              ?type ;\n"
		"       ingen:name     ?name ;\n"
		"       ingen:dataType ?datatype .\n"
		"OPTIONAL { ?port ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"}");

	RDFQuery::Results ports = query.run(filename);

	for (RDFQuery::Results::iterator i = ports.begin(); i != ports.end(); ++i) {
		const Glib::ustring& name     = (*i)["name"];
		const Glib::ustring& type     = _namespaces->qualify((*i)["type"]);
		const Glib::ustring& datatype = (*i)["datatype"];

		if (created.find(name) == created.end()) {
			cerr << "TYPE: " << type << endl;
			bool is_output = (type == "ingen:OutputPort"); // FIXME: check validity
			_engine->create_port(parent.base() + name, datatype, is_output);
			created[name] = true;
		}

		Glib::ustring floatkey = _namespaces->qualify((*i)["floatkey"]);
		Glib::ustring floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(parent.base() + name, floatkey, Atom(val));
		}
	}
}


} // namespace Client
} // namespace Ingen

