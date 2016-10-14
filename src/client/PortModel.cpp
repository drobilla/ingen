/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/client/BlockModel.hpp"
#include "ingen/client/PortModel.hpp"

namespace Ingen {
namespace Client {

void
PortModel::on_property(const Raul::URI& uri, const Atom& value)
{
	if (uri == _uris.ingen_activity) {
		// Don't store activity, it is transient
		signal_activity().emit(value);
		return;
	}

	ObjectModel::on_property(uri, value);

	if (uri == _uris.ingen_value) {
		signal_value_changed().emit(value);
	}
}

bool
PortModel::supports(const URIs::Quark& value_type) const
{
	return has_property(_uris.atom_supports, value_type);
}

bool
PortModel::port_property(const URIs::Quark& uri) const
{
	return has_property(_uris.lv2_portProperty, uri);
}

bool
PortModel::is_uri() const
{
	// FIXME: Resource::has_property doesn't work, URI != URID
	for (auto p : properties()) {
		if (p.second.type() == _uris.atom_URID &&
		    static_cast<LV2_URID>(p.second.get<int32_t>()) == _uris.atom_URID) {
			return true;
		}
	}
	return false;
}

void
PortModel::set(SPtr<ObjectModel> model)
{
	ObjectModel::set(model);

	SPtr<PortModel> port = dynamic_ptr_cast<PortModel>(model);
	if (port) {
		_index = port->_index;
		_direction = port->_direction;
		_signal_value_changed.emit(get_property(_uris.ingen_value));
	}
}

} // namespace Client
} // namespace Ingen
