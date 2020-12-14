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

#include "ingen/client/ObjectModel.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Node.hpp"
#include "ingen/Properties.hpp"
#include "ingen/Resource.hpp"
#include "ingen/URIs.hpp"
#include "ingen/paths.hpp"

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <utility>

namespace ingen {
namespace client {

ObjectModel::ObjectModel(URIs& uris, const Raul::Path& path)
	: Node(uris, path)
	, _path(path)
	, _symbol((path == "/") ? "root" : path.symbol())
{
}

ObjectModel::ObjectModel(const ObjectModel& copy)
	: Node(copy)
	, _parent(copy._parent)
	, _path(copy._path)
	, _symbol(copy._symbol)
{
}

bool
ObjectModel::is_a(const URIs::Quark& type) const
{
	return has_property(_uris.rdf_type, type);
}

void
ObjectModel::on_property(const URI& uri, const Atom& value)
{
	_signal_property.emit(uri, value);
}

void
ObjectModel::on_property_removed(const URI& uri, const Atom& value)
{
	_signal_property_removed.emit(uri, value);
}

const Atom&
ObjectModel::get_property(const URI& key) const
{
	static const Atom null_atom;
	auto i = properties().find(key);
	return (i != properties().end()) ? i->second : null_atom;
}

bool
ObjectModel::polyphonic() const
{
	const Atom& polyphonic = get_property(_uris.ingen_polyphonic);
	return (polyphonic.is_valid() && polyphonic.get<int32_t>());
}

/** Merge the data of `o` with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the value in
 * `o` as correct.  The paths of the two models MUST be equal.
 */
void
ObjectModel::set(const std::shared_ptr<ObjectModel>& o)
{
	assert(_path == o->path());
	if (o->_parent) {
		_parent = o->_parent;
	}

	for (const auto& v : o->properties()) {
		Resource::set_property(v.first, v.second);
		_signal_property.emit(v.first, v.second);
	}
}

void
ObjectModel::set_path(const Raul::Path& p)
{
	_path   = p;
	_symbol = Raul::Symbol(p.is_root() ? "root" : p.symbol());
	set_uri(path_to_uri(p));
	_signal_moved.emit();
}

void
ObjectModel::set_parent(const std::shared_ptr<ObjectModel>& p)
{
	assert(_path.is_child_of(p->path()));
	_parent = p;
}

} // namespace client
} // namespace ingen
