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

#include "Builder.hpp"
#include "common/interface/CommonInterface.hpp"
#include "common/interface/Patch.hpp"
#include "common/interface/Node.hpp"
#include "common/interface/Port.hpp"
#include "common/interface/Connection.hpp"
#include "common/interface/Plugin.hpp"
#include <iostream>
using namespace std;

namespace Ingen {
namespace Shared {


Builder::Builder(CommonInterface& interface)
	: _interface(interface)
{
}


void
Builder::build(SharedPtr<const GraphObject> object)
{
	cout << "BUILDING: " << object->path() << endl;

	SharedPtr<const Patch> patch = PtrCast<const Patch>(object);
	if (patch) {
		if (patch->path() != "/")
		_interface.new_patch(patch->path() + "_copy", patch->internal_polyphony());
		build_object(object);
		for (Patch::Connections::const_iterator i = patch->connections().begin();
				i != patch->connections().end(); ++i)
			_interface.connect((*i)->src_port_path(), (*i)->dst_port_path());
		return;
	}

	SharedPtr<const Node> node = PtrCast<const Node>(object);
	if (node) {
		_interface.new_node(node->path() + "_copy", node->plugin()->uri());
		build_object(object);
		return;
	}

	SharedPtr<const Port> port = PtrCast<const Port>(object);
	if (port) {
		_interface.new_port(port->path() + "_copy", port->index(), port->type().uri(), !port->is_input());
		build_object(object);
		return;
	}
}


void
Builder::build_object(SharedPtr<const GraphObject> object)
{
	for (GraphObject::Variables::const_iterator i = object->variables().begin();
			i != object->variables().end(); ++i) {
		cout << "SETTING " << object->path() << " . " << i->first << endl;
		_interface.set_variable(object->path() + "_copy", i->first, i->second);
	}

	for (GraphObject::Properties::const_iterator i = object->properties().begin();
			i != object->properties().end(); ++i) {
		cout << "SETTING " << object->path() << " . " << i->first << endl;
		_interface.set_property(object->path() + "_copy", i->first, i->second);
	}
}


} // namespace Shared
} // namespace Ingen
