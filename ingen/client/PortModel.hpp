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

#ifndef INGEN_CLIENT_PORTMODEL_HPP
#define INGEN_CLIENT_PORTMODEL_HPP

#include <cstdlib>
#include <string>

#include "ingen/client/ObjectModel.hpp"
#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "lv2/core/lv2.h"
#include "lv2/port-props/port-props.h"

namespace Raul { class Path; }

namespace Ingen {
namespace Client {

/** Model of a port.
 *
 * @ingroup IngenClient
 */
class INGEN_API PortModel : public ObjectModel
{
public:
	enum class Direction { INPUT, OUTPUT };

	GraphType graph_type() const { return Node::GraphType::PORT; }

	bool supports(const URIs::Quark& value_type) const;

	inline uint32_t    index()     const { return _index; }
	inline const Atom& value()     const { return get_property(_uris.ingen_value); }
	inline bool        is_input()  const { return (_direction == Direction::INPUT); }
	inline bool        is_output() const { return (_direction == Direction::OUTPUT); }

	bool port_property(const URIs::Quark& uri) const;

	bool is_logarithmic() const { return port_property(_uris.pprops_logarithmic); }
	bool is_enumeration() const { return port_property(_uris.lv2_enumeration); }
	bool is_integer()     const { return port_property(_uris.lv2_integer); }
	bool is_toggle()      const { return port_property(_uris.lv2_toggled); }
	bool is_numeric()     const {
		return ObjectModel::is_a(_uris.lv2_ControlPort)
			|| ObjectModel::is_a(_uris.lv2_CVPort);
	}
	bool is_uri() const;

	inline bool operator==(const PortModel& pm) const { return (path() == pm.path()); }

	void on_property(const URI& uri, const Atom& value);

	// Signals
	INGEN_SIGNAL(value_changed, void, const Atom&);
	INGEN_SIGNAL(voice_changed, void, uint32_t, const Atom&);
	INGEN_SIGNAL(activity, void, const Atom&);

private:
	friend class ClientStore;

	PortModel(URIs&             uris,
	          const Raul::Path& path,
	          uint32_t          index,
	          Direction         dir)
		: ObjectModel(uris, path)
		, _index(index)
		, _direction(dir)
	{}

	void add_child(SPtr<ObjectModel> c)    { throw; }
	bool remove_child(SPtr<ObjectModel> c) { throw; }

	void set(SPtr<ObjectModel> model);

	uint32_t  _index;
	Direction _direction;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PORTMODEL_HPP
