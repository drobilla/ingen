/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include "ObjectModel.hpp"
#include <raul/TableImpl.hpp>
#include <iostream>

using namespace std;

namespace Ingen {
namespace Client {


ObjectModel::ObjectModel(Store& store, const Path& path, bool polyphonic)
	: _store(store)
	, _path(path)
	, _polyphonic(polyphonic)
{
}


ObjectModel::~ObjectModel()
{
}
	

ObjectModel::const_iterator
ObjectModel::children_begin() const
{
	Store::Objects::const_iterator me = _store.objects().find(_path);
	assert(me != _store.objects().end());
	++me;
	return me;
}


ObjectModel::const_iterator
ObjectModel::children_end() const
{
	Store::Objects::const_iterator me = _store.objects().find(_path);
	assert(me != _store.objects().end());
	return _store.objects().find_descendants_end(me);
}


SharedPtr<ObjectModel>
ObjectModel::find_child(const string& name) const
{
	const_iterator me = _store.objects().find(_path);
	assert(me != _store.objects().end());
	const_iterator children_end = _store.objects().find_descendants_end(me);
	const_iterator child = _store.objects().find(me, children_end, _path.base() + name);
	if (child != _store.objects().end())
		return child->second;
	else
		return SharedPtr<ObjectModel>();
}


/** Get a piece of metadata for this object.
 *
 * @return Metadata value with key @a key, empty string otherwise.
 */
const Atom&
ObjectModel::get_metadata(const string& key) const
{
	static const Atom null_atom;

	MetadataMap::const_iterator i = _metadata.find(key);
	if (i != _metadata.end())
		return i->second;
	else
		return null_atom;
}


void
ObjectModel::add_metadata(const MetadataMap& data)
{
	for (MetadataMap::const_iterator i = data.begin(); i != data.end(); ++i) {
		_metadata[i->first] = i->second;
		signal_metadata.emit(i->first, i->second);
	}
}


void
ObjectModel::set_polyphonic(bool polyphonic)
{
	_polyphonic = polyphonic;
	signal_polyphonic.emit(polyphonic);
}


/** Merge the data of @a model with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the value in
 * @a model as correct.  The paths of the two models MUST be equal.
 */
void
ObjectModel::set(SharedPtr<ObjectModel> model)
{
	assert(_path == model->path());

	for (MetadataMap::const_iterator other = model->metadata().begin();
			other != model->metadata().end(); ++other) {
		
		MetadataMap::const_iterator mine = _metadata.find(other->first);
		
		if (mine != _metadata.end()) {
			cerr << "WARNING:  " << _path << "Client/Server data mismatch: " << other->first << endl;
			cerr << "Setting server value " << other->second;
		}

		_metadata[other->first] = other->second;
		signal_metadata.emit(other->first, other->second);
	}
}


} // namespace Client
} // namespace Ingen

