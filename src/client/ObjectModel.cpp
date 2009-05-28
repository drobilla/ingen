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

#include <iostream>
#include "raul/TableImpl.hpp"
#include "interface/GraphObject.hpp"
#include "ObjectModel.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {


ObjectModel::ObjectModel(const Path& path)
	: ResourceImpl(path)
	, _meta(std::string("meta:#") + path.chop_start("/"))
	, _path(path)
{
}


ObjectModel::~ObjectModel()
{
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
	const Raul::Atom& polyphonic = get_property("ingen:polyphonic");
	return (polyphonic.is_valid() && polyphonic.get_bool());
}


/** Merge the data of @a model with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the value in
 * @a model as correct.  The paths of the two models MUST be equal.
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


} // namespace Client
} // namespace Ingen

