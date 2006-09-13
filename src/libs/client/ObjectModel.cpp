/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "ObjectModel.h"

namespace Ingen {
namespace Client {


ObjectModel::ObjectModel(const Path& path)
: _path(path)
{
}


ObjectModel::~ObjectModel()
{
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
	}
}


/** Merge the data of @a model with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the version in 
 * this as correct.  The paths of the two models must be equal.
 */
void
ObjectModel::assimilate(CountedPtr<ObjectModel> model)
{
	assert(_path == model->path());

	for (MetadataMap::const_iterator i = model->metadata().begin();
			i != model->metadata().end(); ++i) {
		MetadataMap::const_iterator i = _metadata.find(i->first);
		if (i == _metadata.end())
			_metadata[i->first] = i->second;
	}
}


} // namespace Client
} // namespace Ingen

