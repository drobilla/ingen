/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/Edge.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/Patch.hpp"
#include "ingen/Plugin.hpp"
#include "ingen/Port.hpp"
#include "ingen/shared/Builder.hpp"
#include "ingen/shared/URIs.hpp"
#include "raul/Atom.hpp"
#include "raul/Path.hpp"

using namespace std;

namespace Ingen {
namespace Shared {

Builder::Builder(Shared::URIs& uris, Interface& interface)
	: _uris(uris)
	, _interface(interface)
{
}

void
Builder::build(SharedPtr<const GraphObject> object)
{
	SharedPtr<const Patch> patch = PtrCast<const Patch>(object);
	if (patch) {
		if (!object->path().is_root()) {
			Resource::Properties props;
			props.insert(make_pair(_uris.rdf_type,
			                       _uris.forge.alloc_uri(_uris.ingen_Patch.str())));
			props.insert(make_pair(_uris.ingen_polyphony,
			                       _uris.forge.make(int32_t(patch->internal_poly()))));
			_interface.put(object->path(), props);
		}

		build_object(object);
		/*for (Patch::Edges::const_iterator i = patch->edges().begin();
		  i != patch->edges().end(); ++i) {
		  _interface.connect((*i)->src_port_path(), (*i)->dst_port_path());
		  }*/
		return;
	}

	SharedPtr<const Node> node = PtrCast<const Node>(object);
	if (node) {
		Resource::Properties props;
		props.insert(make_pair(_uris.rdf_type, _uris.ingen_Node));
		props.insert(make_pair(_uris.ingen_prototype,
		                       _uris.forge.alloc_uri(node->plugin()->uri().str())));
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
		for (Patch::Edges::const_iterator i = patch->edges().begin();
		     i != patch->edges().end(); ++i) {
			_interface.connect(i->second->tail_path(), i->second->head_path());
		}
		return;
	}
}

void
Builder::build_object(SharedPtr<const GraphObject> object)
{
	typedef GraphObject::Properties::const_iterator iterator;
	_interface.put(object->uri(), object->properties());
}

} // namespace Shared
} // namespace Ingen
