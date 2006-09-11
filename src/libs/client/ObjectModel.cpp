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
: m_path(path)
{
}


/** Get a piece of metadata for this objeect.
 *
 * @return Metadata value with key @a key, empty string otherwise.
 */
string
ObjectModel::get_metadata(const string& key) const
{
	map<string,string>::const_iterator i = m_metadata.find(key);
	if (i != m_metadata.end())
		return i->second;
	else
		return "";
}


void
ObjectModel::set_controller(CountedPtr<ObjectController> c)
{
	m_controller = c;
}


/** Merge the data of @a model with self, as much as possible.
 *
 * This will merge the two models, but with any conflict take the version in 
 * this as correct.  The paths of the two models must be equal.
 */
void
ObjectModel::assimilate(CountedPtr<ObjectModel> model)
{
	assert(m_path == model->path());

	for (map<string,string>::const_iterator i = model->metadata().begin();
			i != model->metadata().end(); ++i) {
		map<string,string>::const_iterator i = m_metadata.find(i->first);
		if (i == m_metadata.end())
			m_metadata[i->first] = i->second;
	}
}


} // namespace Client
} // namespace Ingen

