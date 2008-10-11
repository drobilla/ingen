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

#include "PortModel.hpp"
#include "NodeModel.hpp"

namespace Ingen {
namespace Client {

bool
PortModel::has_hint(const std::string& qname) const
{
	const Atom& hint = get_variable(qname);
	return (hint.is_valid() && hint.get_bool() > 0);
}
	
void
PortModel::set(SharedPtr<ObjectModel> model)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(model);
	if (port) {
		_index = port->_index;
		_type = port->_type;
		_direction = port->_direction;
		_current_val = port->_current_val;
		_connections = port->_connections;
		signal_value_changed.emit(_current_val);
	}

	ObjectModel::set(model);
}

} // namespace Client
} // namespace Ingen
