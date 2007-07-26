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


ObjectModel::ObjectModel(const Path& path)
: _path(path)
{
}


ObjectModel::~ObjectModel()
{
}

SharedPtr<ObjectModel>
ObjectModel::get_child(const string& name) const
{
	assert(name.find("/") == string::npos);
	Children::const_iterator i = _children.find(name);
	return ((i != _children.end()) ? (*i).second : SharedPtr<ObjectModel>());
}

void
ObjectModel::add_child(SharedPtr<ObjectModel> o)
{
	assert(o);
	assert(o->path().is_child_of(_path));
	assert(o->parent().get() == this);
	
#ifndef NDEBUG
	// Be sure there's no duplicates
	Children::iterator existing = _children.find(o->path().name());
	assert(existing == _children.end());
#endif

	_children.insert(make_pair(o->path().name(), o));
	new_child_sig.emit(o);
}

bool
ObjectModel::remove_child(SharedPtr<ObjectModel> o)
{
	assert(o->path().is_child_of(_path));
	assert(o->parent().get() == this);

	Children::iterator i = _children.find(o->path().name());
	if (i != _children.end()) {
		assert(i->second == o);
		_children.erase(i);
		removed_child_sig.emit(o);
		return true;
	} else {
		cerr << "[ObjectModel::remove_child] " << _path
			<< ": failed to find child " << o->path().name() << endl;
		return false;
	}
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
		metadata_update_sig.emit(i->first, i->second);
	}
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
		metadata_update_sig.emit(other->first, other->second);
	}
}


} // namespace Client
} // namespace Ingen

