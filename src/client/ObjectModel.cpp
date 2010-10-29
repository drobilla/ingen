/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "raul/TableImpl.hpp"
#include "interface/GraphObject.hpp"
#include "shared/LV2URIMap.hpp"
#include "ObjectModel.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {


ObjectModel::ObjectModel(Shared::LV2URIMap& uris, const Raul::Path& path)
	: ResourceImpl(uris, path)
	, _meta(uris, ResourceImpl::meta_uri(path))
	, _path(path)
	, _symbol((path == Path::root()) ? "root" : path.symbol())
{
}


ObjectModel::ObjectModel(const ObjectModel& copy)
	: ResourceImpl(copy)
	, _meta(copy._meta)
	, _parent(copy._parent)
	, _path(copy._path)
	, _symbol(copy._symbol)
{
}


ObjectModel::~ObjectModel()
{
}


Raul::Atom&
ObjectModel::set_property(const Raul::URI& key, const Raul::Atom& value)
{
	signal_property.emit(key, value);
	return ResourceImpl::set_property(key, value);
}


Raul::Atom&
ObjectModel::set_meta_property(const Raul::URI& key, const Raul::Atom& value)
{
	signal_property.emit(key, value);
	return _meta.set_property(key, value);
}


void
ObjectModel::add_property(const Raul::URI& key, const Raul::Atom& value)
{
	ResourceImpl::add_property(key, value);
	signal_property.emit(key, value);
}


const Atom&
ObjectModel::get_property(const Raul::URI& key) const
{
	Resource::Properties::const_iterator i = properties().find(key);
	return (i != properties().end()) ? i->second : _meta.get_property(key);
}


bool
ObjectModel::polyphonic() const
{
	const Raul::Atom& polyphonic = get_property(_uris.ingen_polyphonic);
	return (polyphonic.is_valid() && polyphonic.get_bool());
}


/** Merge the data of @a model with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the value in
 * @a o as correct.  The paths of the two models MUST be equal.
 */
void
ObjectModel::set(SharedPtr<ObjectModel> o)
{
	assert(_path == o->path());
	if (o->_parent)
		_parent = o->_parent;

	for (Properties::const_iterator v = o->meta().properties().begin(); v != o->meta().properties().end(); ++v) {
		o->meta().set_property(v->first, v->second);
		signal_property.emit(v->first, v->second);
	}
	for (Properties::const_iterator v = o->properties().begin(); v != o->properties().end(); ++v) {
		ResourceImpl::set_property(v->first, v->second);
		signal_property.emit(v->first, v->second);
	}
}


void
ObjectModel::set_path(const Raul::Path& p)
{
	_path = p;
	_symbol = p.symbol();
	_meta.set_uri(ResourceImpl::meta_uri(p));
	signal_moved.emit();
}


void
ObjectModel::set_parent(SharedPtr<ObjectModel> p)
{
	assert(_path.is_child_of(p->path()));
	_parent = p;
}


} // namespace Client
} // namespace Ingen

