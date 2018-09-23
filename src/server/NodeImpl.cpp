/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GraphImpl.hpp"
#include "NodeImpl.hpp"
#include "ThreadManager.hpp"

namespace ingen {
namespace server {

NodeImpl::NodeImpl(const ingen::URIs&  uris,
                   NodeImpl*           parent,
                   const Raul::Symbol& symbol)
	: Node(uris, parent ? parent->path().child(symbol) : Raul::Path("/"))
	, _parent(parent)
	, _path(parent ? parent->path().child(symbol) : Raul::Path("/"))
	, _symbol(symbol)
{
}

const Atom&
NodeImpl::get_property(const URI& key) const
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
	static const Atom null_atom;
	auto i = properties().find(key);
	return (i != properties().end()) ? i->second : null_atom;
}

GraphImpl*
NodeImpl::parent_graph() const
{
	return dynamic_cast<GraphImpl*>((BlockImpl*)_parent);
}

} // namespace server
} // namespace ingen
