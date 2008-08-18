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
Builder::build(const Raul::Path& prefix, SharedPtr<const GraphObject> object)
{
	SharedPtr<const Patch> patch = PtrCast<const Patch>(object);
	if (patch) {
		if (object->path() != "/") {
			const std::string path_str = prefix + object->path();
			//cout << "BUILDING PATCH " << path_str << endl;
			_interface.new_patch(path_str, patch->internal_polyphony());
		}

		build_object(prefix, object);
		for (Patch::Connections::const_iterator i = patch->connections().begin();
				i != patch->connections().end(); ++i) {
			_interface.connect(prefix.base() + (*i)->src_port_path().substr(1),
			                   prefix.base() + (*i)->dst_port_path().substr(1));
		}
		return;
	}

	SharedPtr<const Node> node = PtrCast<const Node>(object);
	if (node) {
		Raul::Path path = prefix.base() + node->path().substr(1);
		//cout << "BUILDING NODE " << path << endl;
		_interface.new_node(path, node->plugin()->uri());
		build_object(prefix, object);
		return;
	}

	SharedPtr<const Port> port = PtrCast<const Port>(object);
	if (port) {
		Raul::Path path = prefix.base() + port->path().substr(1);
		//cout << "BUILDING PORT " << path << endl;
		_interface.new_port(path, port->index(), port->type().uri(), !port->is_input());
		build_object(prefix, object);
		return;
	}
}


void
Builder::build_object(const Raul::Path& prefix, SharedPtr<const GraphObject> object)
{
	for (GraphObject::Variables::const_iterator i = object->variables().begin();
			i != object->variables().end(); ++i)
		_interface.set_variable(prefix.base() + object->path().substr(1), i->first, i->second);

	for (GraphObject::Properties::const_iterator i = object->properties().begin();
			i != object->properties().end(); ++i) {
		if (object->path() == "/")
			continue;
		string path_str = prefix.base() + object->path().substr(1);
		_interface.set_property(prefix.base() + object->path().substr(1), i->first, i->second);
	}
}


} // namespace Shared
} // namespace Ingen
