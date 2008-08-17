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

#ifndef PORTMODEL_H
#define PORTMODEL_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <sigc++/sigc++.h>
#include <raul/SharedPtr.hpp>
#include <raul/Path.hpp>
#include "interface/Port.hpp"
#include "ObjectModel.hpp"

using std::string; using std::vector;

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
	
	inline uint32_t    index()     const { return _index; } 
	inline DataType    type()      const { return _type; }
	inline const Atom& value()     const { return _current_val; }
	inline bool        connected() const { return (_connections > 0); }
	inline bool        is_input()  const { return (_direction == INPUT); }
	inline bool        is_output() const { return (_direction == OUTPUT); }
	
	bool is_logarithmic() const;
	bool is_integer()     const;
	bool is_toggle()      const;

	inline bool operator==(const PortModel& pm) const { return (_path == pm._path); }
	
	inline void value(const Atom& val)
	{
		if (val != _current_val) {
			_current_val = val;
			signal_value_changed.emit(val);
		}
	}
	
	// Signals
	sigc::signal<void, const Atom&>           signal_value_changed; ///< Value ports
	sigc::signal<void>                        signal_activity; ///< Message ports
	sigc::signal<void, SharedPtr<PortModel> > signal_connection;
	sigc::signal<void, SharedPtr<PortModel> > signal_disconnection;

private:
	friend class ClientStore;
	
	PortModel(const Path& path, uint32_t index, DataType type, Direction dir)
		: ObjectModel(path)
		, _index(index)
		, _type(type)
		, _direction(dir)
		, _current_val(0.0f)
		, _connections(0)
	{
		if (_type == DataType::UNKNOWN)
			std::cerr << "[PortModel] Warning: Unknown port type" << std::endl;
	}
	
	void add_child(SharedPtr<ObjectModel> c)    { throw; }
	bool remove_child(SharedPtr<ObjectModel> c) { throw; }
	
	void connected_to(SharedPtr<PortModel> p)      { ++_connections; signal_connection.emit(p); }
	void disconnected_from(SharedPtr<PortModel> p) { --_connections; signal_disconnection.emit(p); }
	
	void set(SharedPtr<ObjectModel> model);
	
	uint32_t  _index;
	DataType  _type;
	Direction _direction;
	Atom      _current_val;
	size_t    _connections;
};

typedef vector<SharedPtr<PortModel> > PortModelList;


} // namespace Client
} // namespace Ingen

#endif // PORTMODEL_H
