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

#include "ingen/Arc.hpp"
#include "ingen/Atom.hpp"
#include "ingen/Builder.hpp"
#include "ingen/Interface.hpp"
#include "ingen/Node.hpp"
#include "ingen/URIs.hpp"
#include "raul/Path.hpp"

using namespace std;

namespace Ingen {

Builder::Builder(URIs& uris, Interface& interface)
	: _uris(uris)
	, _interface(interface)
{
}

void
Builder::build(SPtr<const Node> object)
{
	_interface.put(object->uri(), object->properties());
}

void
Builder::connect(SPtr<const Node> object)
{
	if (object->graph_type() == Node::GraphType::GRAPH) {
		for (auto a : object->arcs()) {
			_interface.connect(a.second->tail_path(), a.second->head_path());
		}
		return;
	}
}

} // namespace Ingen
