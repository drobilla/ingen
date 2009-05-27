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

#include "raul/Atom.hpp"
#include "Builder.hpp"
#include "common/interface/CommonInterface.hpp"
#include "common/interface/Patch.hpp"
#include "common/interface/Node.hpp"
#include "common/interface/Port.hpp"
#include "common/interface/Connection.hpp"
#include "common/interface/Plugin.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


Builder::Builder(CommonInterface& interface)
	: _interface(interface)
{
}


void
Builder::build(SharedPtr<const GraphObject> object)
{
	SharedPtr<const Patch> patch = PtrCast<const Patch>(object);
	if (patch) {
		if (!object->path().is_root()) {
			Resource::Properties props;
			props.insert(make_pair("rdf:type",        Atom(Atom::URI, "ingen:Patch")));
			props.insert(make_pair("ingen:polyphony", Atom(int32_t(patch->internal_polyphony()))));
			_interface.put(object->path(), props);
		}

		build_object(object);
		/*for (Patch::Connections::const_iterator i = patch->connections().begin();
				i != patch->connections().end(); ++i) {
			_interface.connect((*i)->src_port_path(), (*i)->dst_port_path());
		}*/
		return;
	}

	SharedPtr<const Node> node = PtrCast<const Node>(object);
	if (node) {
		Resource::Properties props;
		props.insert(make_pair("rdf:type",       Atom(Atom::URI, "ingen:Node")));
		props.insert(make_pair("rdf:instanceOf", Atom(Atom::URI, node->plugin()->uri().str())));
		_interface.put(node->path(), props);
		build_object(object);
		return;
	}

	SharedPtr<const Port> port = PtrCast<const Port>(object);
	if (port) {
		_interface.put(port->path(), port->properties());
		build_object(object);
		return;
	}
}


void
Builder::connect(SharedPtr<const GraphObject> object)
{
	SharedPtr<const Patch> patch = PtrCast<const Patch>(object);
	if (patch) {
		for (Patch::Connections::const_iterator i = patch->connections().begin();
				i != patch->connections().end(); ++i) {
			_interface.connect((*i)->src_port_path(), (*i)->dst_port_path());
		}
		return;
	}
}


void
Builder::build_object(SharedPtr<const GraphObject> object)
{
	for (GraphObject::Properties::const_iterator i = object->variables().begin();
			i != object->variables().end(); ++i)
		_interface.set_variable(object->path(), i->first, i->second);

	for (GraphObject::Properties::const_iterator i = object->properties().begin();
			i != object->properties().end(); ++i) {
		if (object->path().is_root())
			continue;
		_interface.set_property(object->path(), i->first, i->second);
	}
}


} // namespace Shared
} // namespace Ingen
