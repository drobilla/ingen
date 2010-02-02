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

#ifndef INGEN_CLIENT_PORTMODEL_HPP
#define INGEN_CLIENT_PORTMODEL_HPP

#include <cstdlib>
#include <string>
#include <sigc++/sigc++.h>
#include "raul/log.hpp"
#include "raul/SharedPtr.hpp"
#include "interface/Port.hpp"
#include "ObjectModel.hpp"

#include <stdio.h>


namespace Raul { class Path; }

namespace Ingen {
namespace Client {


/** Model of a port.
 *
 * \ingroup IngenClient
 */
class PortModel : public ObjectModel, public Ingen::Shared::Port
{
public:
	enum Direction { INPUT, OUTPUT };

	inline uint32_t          index()     const { return _index; }
	inline Shared::PortType  type()      const { return _type; }
	inline const Raul::Atom& value()     const { return _current_val; }
	inline bool              connected() const { return (_connections > 0); }
	inline bool              is_input()  const { return (_direction == INPUT); }
	inline bool              is_output() const { return (_direction == OUTPUT); }

	bool has_hint(const std::string& qname) const;

	bool is_logarithmic() const { return has_hint("ingen:logarithmic"); }
	bool is_integer()     const { return has_hint("lv2:integer"); }
	bool is_toggle()      const { return has_hint("lv2:toggled"); }

	inline bool operator==(const PortModel& pm) const { return (_path == pm._path); }

	Raul::Atom& set_property(const Raul::URI&  uri,
	                         const Raul::Atom& value) {
		Raul::Atom& ret = ObjectModel::set_property(uri, value);
		if (uri.str() == "ingen:value")
			this->value(value);
		return ret;
	}

	inline void value(const Raul::Atom& val) {
		if (val != _current_val) {
			_current_val = val;
			signal_value_changed.emit(val);
		}
	}

	inline void value(uint32_t voice, const Raul::Atom& val) {
		// FIXME: implement properly
		signal_voice_changed.emit(voice, val);
	}

	// Signals
	sigc::signal<void, const Raul::Atom&>           signal_value_changed; ///< Value ports
	sigc::signal<void, uint32_t, const Raul::Atom&> signal_voice_changed; ///< Polyphonic value ports
	sigc::signal<void>                              signal_activity; ///< Message ports
	sigc::signal<void, SharedPtr<PortModel> >       signal_connection;
	sigc::signal<void, SharedPtr<PortModel> >       signal_disconnection;

private:
	friend class ClientStore;

	PortModel(const Raul::Path& path, uint32_t index, Shared::PortType type, Direction dir)
		: ObjectModel(path)
		, _index(index)
		, _type(type)
		, _direction(dir)
		, _current_val(0.0f)
		, _connections(0)
	{
		if (_type == Shared::PortType::UNKNOWN)
			Raul::warn << "[PortModel] Unknown port type" << std::endl;
	}

	void add_child(SharedPtr<ObjectModel> c)    { throw; }
	bool remove_child(SharedPtr<ObjectModel> c) { throw; }

	void connected_to(SharedPtr<PortModel> p)      { ++_connections; signal_connection.emit(p); }
	void disconnected_from(SharedPtr<PortModel> p) { --_connections; signal_disconnection.emit(p); }

	void set(SharedPtr<ObjectModel> model);

	uint32_t         _index;
	Shared::PortType _type;
	Direction        _direction;
	Raul::Atom       _current_val;
	size_t           _connections;
};


} // namespace Client
} // namespace Ingen

#endif // INGEN_CLIENT_PORTMODEL_HPP
