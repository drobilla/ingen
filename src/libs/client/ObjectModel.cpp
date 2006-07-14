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

namespace LibOmClient {


ObjectModel::ObjectModel(const string& path)
: m_path(path),
  m_parent(NULL),
  m_controller(NULL)
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
		return (*i).second;
	else
		return "";
}


/** The base path for children of this Object.
 * 
 * (This is here to avoid needing special cases for the root patch everywhere).
 */
string
ObjectModel::base_path() const
{
	return (path() == "/") ? "/" : path() + "/";
}


void
ObjectModel::set_controller(ObjectController* c)
{
	assert(m_controller == NULL);
	m_controller = c;
}

} // namespace LibOmClient

