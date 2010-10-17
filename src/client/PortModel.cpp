/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "shared/LV2URIMap.hpp"
#include "PortModel.hpp"
#include "NodeModel.hpp"


namespace Ingen {
namespace Client {


Raul::Atom&
PortModel::set_property(const Raul::URI&  uri,
                        const Raul::Atom& value)
{
	Raul::Atom& ret = ObjectModel::set_property(uri, value);
	if (uri == _uris.ingen_value)
		this->value(value);
	return ret;
}


bool
PortModel::supports(const Raul::URI& value_type) const
{
	return has_property(_uris.aport_supports, value_type);
}


bool
PortModel::port_property(const std::string& uri) const
{
	return has_property(_uris.lv2_portProperty, Raul::URI(uri));
}


void
PortModel::set(SharedPtr<ObjectModel> model)
{
	SharedPtr<PortModel> port = PtrCast<PortModel>(model);
	if (port) {
		_index = port->_index;
		_types = port->_types;
		_direction = port->_direction;
		_current_val = port->_current_val;
		_connections = port->_connections;
		signal_value_changed.emit(_current_val);
	}

	ObjectModel::set(model);
}


bool
PortModel::has_context(const Raul::URI& uri)
{
	const Raul::Atom& context = get_property(_uris.ctx_context);
	if (uri == _uris.ctx_AudioContext && !context.is_valid())
		return true;
	else
		return context == uri;
}


} // namespace Client
} // namespace Ingen
