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

#include "ingen/GraphObject.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/URIs.hpp"
#include "raul/TableImpl.hpp"

namespace Ingen {
namespace Client {

ObjectModel::ObjectModel(URIs& uris, const Raul::Path& path)
	: GraphObject(uris, path)
	, _path(path)
	, _symbol((path == "/") ? "root" : path.symbol())
{
}

ObjectModel::ObjectModel(const ObjectModel& copy)
	: GraphObject(copy)
	, _parent(copy._parent)
	, _path(copy._path)
	, _symbol(copy._symbol)
{
}

ObjectModel::~ObjectModel()
{
}

bool
ObjectModel::is_a(const Raul::URI& type) const
{
	return has_property(_uris.rdf_type, _uris.forge.alloc_uri(type));
}

void
ObjectModel::on_property(const Raul::URI& uri, const Raul::Atom& value)
{
	_signal_property.emit(uri, value);
}

const Raul::Atom&
ObjectModel::get_property(const Raul::URI& key) const
{
	static const Raul::Atom null_atom;
	Resource::Properties::const_iterator i = properties().find(key);
	return (i != properties().end()) ? i->second : null_atom;
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

	for (Properties::const_iterator v = o->properties().begin();
	     v != o->properties().end(); ++v) {
		Resource::set_property(v->first, v->second);
		_signal_property.emit(v->first, v->second);
	}
}

void
ObjectModel::set_path(const Raul::Path& p)
{
	_path   = p;
	_symbol = Raul::Symbol(p.is_root() ? "root" : p.symbol());
	_signal_moved.emit();
}

void
ObjectModel::set_parent(SharedPtr<ObjectModel> p)
{
	assert(_path.is_child_of(p->path()));
	_parent = p;
}

} // namespace Client
} // namespace Ingen

