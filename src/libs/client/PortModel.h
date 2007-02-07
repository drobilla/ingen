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
#include <list>
#include <sigc++/sigc++.h>
#include "ObjectModel.h"
#include "raul/SharedPtr.h"
#include "raul/Path.h"
using std::string; using std::list; using std::cerr; using std::endl;

namespace Ingen {
namespace Client {


/** Model of a port.
 *
 * \ingroup IngenClient.
 */
class PortModel : public ObjectModel
{
public:
	enum Direction { INPUT, OUTPUT };
	
	// FIXME: metadataify
	enum Hint      { NONE, INTEGER, TOGGLE, LOGARITHMIC };
	
	inline string type()           const { return _type; }
	inline float  value()          const { return _current_val; }
	inline bool   connected()      const { return (_connections > 0); }
	inline bool   is_input()       const { return (_direction == INPUT); }
	inline bool   is_output()      const { return (_direction == OUTPUT); }
	inline bool   is_audio()       const { return (_type == "ingen:audio"); }
	inline bool   is_control()     const { return (_type == "ingen:control"); }
	inline bool   is_midi()        const { return (_type == "ingen:midi"); }
	inline bool   is_logarithmic() const { return (_hint == LOGARITHMIC); }
	inline bool   is_integer()     const { return (_hint == INTEGER); }
	inline bool   is_toggle()      const { return (_hint == TOGGLE); }
	
	inline bool operator==(const PortModel& pm) const { return (_path == pm._path); }
	
	inline void value(float val)
	{
		if (val != _current_val) {
			_current_val = val;
			control_change_sig.emit(val);
		}
	}

	// Signals
	sigc::signal<void, float>                 control_change_sig; ///< "Control" ports only
	sigc::signal<void, SharedPtr<PortModel> > connection_sig;
	sigc::signal<void, SharedPtr<PortModel> > disconnection_sig;

private:
	friend class Store;
	
	PortModel(const Path& path, const string& type, Direction dir, Hint hint)
	: ObjectModel(path),
	  _type(type),
	  _direction(dir),
	  _hint(hint),
	  _current_val(0.0f),
	  _connections(0)
	{
		if (!is_audio() && !is_control() && !is_input())
			cerr << "[PortModel] Warning: Unknown port type" << endl;
	}
	
	PortModel(const Path& path, const string& type, Direction dir)
	: ObjectModel(path),
	  _type(type),
	  _direction(dir),
	  _hint(NONE),
	  _current_val(0.0f),
	  _connections(0)
	{
		if (!is_audio() && !is_control() && !is_input())
			cerr << "[PortModel] Warning: Unknown port type" << endl;
	}

	void add_child(SharedPtr<ObjectModel> c)    { throw; }
	void remove_child(SharedPtr<ObjectModel> c) { throw; }
	
	void connected_to(SharedPtr<PortModel> p)      { ++_connections; connection_sig.emit(p); }
	void disconnected_from(SharedPtr<PortModel> p) { --_connections; disconnection_sig.emit(p); }
	
	string    _type;
	Direction _direction;
	Hint      _hint;
	float     _current_val;
	size_t    _connections;
};

typedef list<SharedPtr<PortModel> > PortModelList;


} // namespace Client
} // namespace Ingen

#endif // PORTMODEL_H
