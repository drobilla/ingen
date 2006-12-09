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

	RDFQuery query(Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?floatkey ?floatval FROM <") + filename + "> WHERE {\n"
		"?patch ingen:node   ?node .\n"
		"?node  ingen:name   ?name ;\n"
		"       ingen:plugin ?plugin ;\n"
		"OPTIONAL { ?node ?floatkey  ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal )\n"
		"         }\n"
		"}");

	RDFQuery::Results nodes = query.run(filename);

	for (RDFQuery::Results::iterator i = nodes.begin(); i != nodes.end(); ++i) {
		const Glib::ustring& name   = (*i)["name"];
		const Glib::ustring& plugin = (*i)["plugin"];

		if (created.find(name) == created.end()) {
			cerr << "CREATING " << name << endl;
			_engine->create_node(parent.base() + name, plugin, false);
			created[name] = true;
		}

		Glib::ustring floatkey = _namespaces->qualify((*i)["floatkey"]);
		Glib::ustring floatval = (*i)["floatval"];

		float val = atof(floatval.c_str());

		cerr << floatkey << " = " << val << endl;

		_engine->set_metadata(parent.base() + name, floatkey, Atom(val));
	}
	
}


} // namespace Client
} // namespace Ingen

