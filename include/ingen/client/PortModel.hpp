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

#ifndef INGEN_CLIENT_PORTMODEL_HPP
#define INGEN_CLIENT_PORTMODEL_HPP

#include <cstdlib>
#include <string>

#include "raul/SharedPtr.hpp"
#include "raul/log.hpp"

#include "ingen/Port.hpp"
#include "ingen/client/ObjectModel.hpp"

namespace Raul { class Path; }

namespace Ingen {
namespace Client {

/** Model of a port.
 *
 * \ingroup IngenClient
 */
class PortModel : public ObjectModel, public Ingen::Port
{
public:
	enum Direction { INPUT, OUTPUT };

	bool supports(const Raul::URI& value_type) const;

	inline uint32_t          index()     const { return _index; }
	inline const Raul::Atom& value()     const { return _current_val; }
	inline bool              connected() const { return (_connections > 0); }
	inline bool              is_input()  const { return (_direction == INPUT); }
	inline bool              is_output() const { return (_direction == OUTPUT); }

	bool port_property(const Raul::URI& uri) const;

	bool is_numeric()     const { return ObjectModel::is_a("http://lv2plug.in/ns/lv2core#ControlPort"); }
	bool is_logarithmic() const { return port_property("http://drobilla.net/ns/ingen#logarithmic"); }
	bool is_integer()     const { return port_property("http://lv2plug.in/ns/lv2core#integer"); }
	bool is_toggle()      const { return port_property("http://lv2plug.in/ns/lv2core#toggled"); }

	bool has_context(const Raul::URI& context) const;


	inline bool operator==(const PortModel& pm) const { return (path() == pm.path()); }

	const Raul::Atom& set_property(const Raul::URI&  uri,
	                               const Raul::Atom& value,
	                               Resource::Graph   ctx);

	inline void value(const Raul::Atom& val) {
		if (val != _current_val) {
			_current_val = val;
			_signal_value_changed.emit(val);
		}
	}

	inline void value(uint32_t voice, const Raul::Atom& val) {
		// FIXME: implement properly
		_signal_voice_changed.emit(voice, val);
	}

	// Signals
	INGEN_SIGNAL(value_changed, void, const Raul::Atom&);
	INGEN_SIGNAL(voice_changed, void, uint32_t, const Raul::Atom&);
	INGEN_SIGNAL(activity, void, const Raul::Atom&);
	INGEN_SIGNAL(connection, void, SharedPtr<PortModel>);
	INGEN_SIGNAL(disconnection, void, SharedPtr<PortModel>);

private:
	friend class ClientStore;

	PortModel(Shared::URIs& uris,
	          const Raul::Path&  path,
	          uint32_t           index,
	          Direction          dir)
		: ObjectModel(uris, path)
		, _index(index)
		, _direction(dir)
		, _current_val(0.0f)
		, _connections(0)
	{}

	void add_child(SharedPtr<ObjectModel> c)    { throw; }
	bool remove_child(SharedPtr<ObjectModel> c) { throw; }

	void connected_to(SharedPtr<PortModel> p)      { ++_connections; _signal_connection.emit(p); }
	void disconnected_from(SharedPtr<PortModel> p) { --_connections; _signal_disconnection.emit(p); }

	void set(SharedPtr<ObjectModel> model);

	uint32_t           _index;
	Direction          _direction;
	Raul::Atom         _current_val;
	size_t             _connections;
};

} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PORTMODEL_HPP
