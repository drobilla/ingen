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

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/client/PortModel.hpp"
#include "ingen/client/NodeModel.hpp"

namespace Ingen {
namespace Client {

void
PortModel::on_property(const Raul::URI& uri, const Raul::Atom& value)
{
	if (uri == _uris.ingen_value) {
		this->value(value);
	}
}

bool
PortModel::supports(const Raul::URI& value_type) const
{
	return has_property(_uris.atom_supports, value_type);
}

bool
PortModel::port_property(const Raul::URI& uri) const
{
	return has_property(_uris.lv2_portProperty, uri);
}

void
PortModel::set(SharedPtr<ObjectModel> model)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(model);
	if (port) {
		_index = port->_index;
		_direction = port->_direction;
		_current_val = port->_current_val;
		_connections = port->_connections;
		_signal_value_changed.emit(_current_val);
	}

	ObjectModel::set(model);
}

bool
PortModel::has_context(const Raul::URI& uri) const
{
	const Raul::Atom& context = get_property(_uris.ctx_context);
	if (uri == _uris.ctx_audioContext && !context.is_valid())
		return true;
	else
		return context == uri;
}

} // namespace Client
} // namespace Ingen
