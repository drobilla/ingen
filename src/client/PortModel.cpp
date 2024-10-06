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

#include "ingen/client/PortModel.hpp"

#include "ingen/Properties.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "ingen/client/ObjectModel.hpp"
#include "lv2/urid/urid.h"

#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <utility>

namespace ingen::client {

void
PortModel::on_property(const URI& uri, const Atom& value)
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
	for (const auto& p : properties()) {
		if (p.second.type() == _uris.atom_URID &&
		    static_cast<LV2_URID>(p.second.get<int32_t>()) == _uris.atom_URID) {
			return true;
		}
	}
	return false;
}

void
PortModel::add_child(const std::shared_ptr<ObjectModel>&)
{
	std::terminate();
}

bool
PortModel::remove_child(const std::shared_ptr<ObjectModel>&)
{
	std::terminate();
}

void
PortModel::set(const std::shared_ptr<ObjectModel>& model)
{
	ObjectModel::set(model);

	auto port = std::dynamic_pointer_cast<PortModel>(model);
	if (port) {
		_index = port->_index;
		_direction = port->_direction;
		_signal_value_changed.emit(get_property(_uris.ingen_value));
	}
}

} // namespace ingen::client
