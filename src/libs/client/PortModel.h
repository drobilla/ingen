/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
#include <string>
#include <list>
#include <sigc++/sigc++.h>
#include "ObjectModel.h"
#include "util/CountedPtr.h"
using std::string; using std::list;

namespace Ingen {
namespace Client {

/* Model of a port.
 *
 * \ingroup IngenClient.
 */
class PortModel : public ObjectModel
{
public:
	// FIXME: metadataify
	enum Type      { CONTROL, AUDIO, MIDI };
	enum Direction { INPUT, OUTPUT };
	enum Hint      { NONE, INTEGER, TOGGLE, LOGARITHMIC };
	
	PortModel(const string& path, Type type, Direction dir, Hint hint)
	: ObjectModel(path),
	  m_type(type),
	  m_direction(dir),
	  m_hint(hint),
	  m_current_val(0.0f),
	  m_connections(0)
	{
	}
	
	PortModel(const string& path, Type type, Direction dir)
	: ObjectModel(path),
	  m_type(type),
	  m_direction(dir),
	  m_hint(NONE),
	  m_current_val(0.0f),
	  m_connections(0)
	{
	}
	
	void add_child(CountedPtr<ObjectModel> c)    { throw; }
	void remove_child(CountedPtr<ObjectModel> c) { throw; }
	
	inline float value() const        { return m_current_val; }
	inline void  value(float f)       { m_current_val = f; control_change_sig.emit(f); }
	inline bool  connected()          { return (m_connections > 0); }
	inline Type  type()               { return m_type; }

	inline bool is_input()       const { return (m_direction == INPUT); }
	inline bool is_output()      const { return (m_direction == OUTPUT); }
	inline bool is_audio()       const { return (m_type == AUDIO); }
	inline bool is_control()     const { return (m_type == CONTROL); }
	inline bool is_midi()        const { return (m_type == MIDI); }
	inline bool is_logarithmic() const { return (m_hint == LOGARITHMIC); }
	inline bool is_integer()     const { return (m_hint == INTEGER); }
	inline bool is_toggle()      const { return (m_hint == TOGGLE); }
	
	inline bool operator==(const PortModel& pm)
		{ return (_path == pm._path); }

	void connected_to(CountedPtr<PortModel> p)      { ++m_connections; connection_sig.emit(p); }
	void disconnected_from(CountedPtr<PortModel> p) { --m_connections; disconnection_sig.emit(p); }
	
	// Signals
	sigc::signal<void, float>                  control_change_sig; ///< "Control" ports only
	sigc::signal<void, CountedPtr<PortModel> > connection_sig;
	sigc::signal<void, CountedPtr<PortModel> > disconnection_sig;

private:
	// Prevent copies (undefined)
	PortModel(const PortModel& copy);
	PortModel& operator=(const PortModel& copy);
	
	Type      m_type;
	Direction m_direction;
	Hint      m_hint;
	float     m_current_val;
	size_t    m_connections;
};

typedef list<CountedPtr<PortModel> > PortModelList;


} // namespace Client
} // namespace Ingen

#endif // PORTMODEL_H
