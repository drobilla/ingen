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

#include "raul/Atom.hpp"
#include "GraphObjectImpl.hpp"
#include "PatchImpl.hpp"
#include "EngineStore.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {

using namespace Shared;

void
GraphObjectImpl::set_variable(const Raul::URI& key, const Atom& value)
{
	// Ignore duplicate statements
	typedef Resource::Properties::const_iterator iterator;
	const std::pair<iterator,iterator> range = _variables.equal_range(key);
	for (iterator i = range.first; i != range.second; ++i)
		if (i->second == value)
			return;

	_variables.insert(make_pair(key, value));
}


const Atom&
GraphObjectImpl::get_variable(const Raul::URI& key)
{
	static const Atom null_atom;
	Properties::iterator i = _variables.find(key);
	return (i != _variables.end()) ? (*i).second : null_atom;
}


PatchImpl*
GraphObjectImpl::parent_patch() const
{
	return dynamic_cast<PatchImpl*>((NodeImpl*)_parent);
}


SharedPtr<GraphObject>
GraphObjectImpl::find_child(const string& name) const
{
	throw;
}


} // namespace Ingen
