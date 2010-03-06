/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "EngineStore.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;


GraphObjectImpl::GraphObjectImpl(Shared::LV2URIMap& uris,
		GraphObjectImpl* parent, const Symbol& symbol)
	: ResourceImpl(uris, parent ? parent->path().child(symbol) : Raul::Path::root())
	, _parent(parent)
	, _path(parent ? parent->path().child(symbol) : "/")
	, _symbol(symbol)
	, _meta(uris, ResourceImpl::meta_uri(uri()))
{
}


void
GraphObjectImpl::add_meta_property(const Raul::URI& key, const Atom& value)
{
	_meta.add_property(key, value);
}


void
GraphObjectImpl::set_meta_property(const Raul::URI& key, const Atom& value)
{
	_meta.set_property(key, value);
}


const Atom&
GraphObjectImpl::get_property(const Raul::URI& key) const
{
	Resource::Properties::const_iterator i = properties().find(key);
	return (i != properties().end()) ? i->second : _meta.get_property(key);
}


PatchImpl*
GraphObjectImpl::parent_patch() const
{
	return dynamic_cast<PatchImpl*>((NodeImpl*)_parent);
}


SharedPtr<GraphObject>
GraphObjectImpl::find_child(const string& name) const
{
	throw;
}


} // namespace Ingen
