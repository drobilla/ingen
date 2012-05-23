/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/client/NodeModel.hpp"
#include "ingen/client/PortModel.hpp"

namespace Ingen {
namespace Client {

void
PortModel::on_property(const Raul::URI& uri, const Raul::Atom& value)
{
	if (uri == _uris.ingen_activity) {
		signal_activity().emit(value);
		return;
	} else {
		if (uri == _uris.ingen_value) {
			this->value(value);
		}
	}
	ObjectModel::on_property(uri, value);
}

bool
PortModel::supports(const Raul::URI& value_type) const
{
	return has_property(_uris.atom_supports,
	                    _uris.forge.alloc_uri(value_type.str()));
}

bool
PortModel::port_property(const Raul::URI& uri) const
{
	return has_property(_uris.lv2_portProperty,
	                    _uris.forge.alloc_uri(uri.str()));
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

} // namespace Client
} // namespace Ingen
