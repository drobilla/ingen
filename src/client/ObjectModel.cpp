/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#include "ingen/GraphObject.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/URIs.hpp"
#include "raul/TableImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

ObjectModel::ObjectModel(Shared::URIs& uris, const Raul::Path& path)
	: ResourceImpl(uris, path)
	, _meta(uris, Raul::URI("http://example.org/FIXME"))
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

bool
ObjectModel::is_a(const Raul::URI& type) const
{
	return has_property(_uris.rdf_type, type);
}

void
ObjectModel::on_property(const Raul::URI& uri, const Raul::Atom& value)
{
	_signal_property.emit(uri, value);
}

const Atom&
ObjectModel::get_property(const Raul::URI& key) const
{
	static const Atom null_atom;
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
		ResourceImpl::set_property(v->first, v->second);
		_signal_property.emit(v->first, v->second);
	}
}

void
ObjectModel::set_path(const Raul::Path& p)
{
	_path   = p;
	_symbol = p.symbol();
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

