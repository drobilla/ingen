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
#include "raul/SharedPtr.h"
#include "raul/Path.h"
using std::string; using std::list;

namespace Ingen {
namespace Client {


/** Model of a port.
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
	
	inline float value()          const { return m_current_val; }
	inline bool  connected()      const { return (m_connections > 0); }
	inline Type  type()           const { return m_type; }
	inline bool  is_input()       const { return (m_direction == INPUT); }
	inline bool  is_output()      const { return (m_direction == OUTPUT); }
	inline bool  is_audio()       const { return (m_type == AUDIO); }
	inline bool  is_control()     const { return (m_type == CONTROL); }
	inline bool  is_midi()        const { return (m_type == MIDI); }
	inline bool  is_logarithmic() const { return (m_hint == LOGARITHMIC); }
	inline bool  is_integer()     const { return (m_hint == INTEGER); }
	inline bool  is_toggle()      const { return (m_hint == TOGGLE); }
	
	inline bool operator==(const PortModel& pm) const { return (_path == pm._path); }

	// Signals
	sigc::signal<void, float>                  control_change_sig; ///< "Control" ports only
	sigc::signal<void, SharedPtr<PortModel> > connection_sig;
	sigc::signal<void, SharedPtr<PortModel> > disconnection_sig;

private:
	friend class Store;
	
	PortModel(const Path& path, Type type, Direction dir, Hint hint)
	: ObjectModel(path),
	  m_type(type),
	  m_direction(dir),
	  m_hint(hint),
	  m_current_val(0.0f),
	  m_connections(0)
	{
	}
	
	PortModel(const Path& path, Type type, Direction dir)
	: ObjectModel(path),
	  m_type(type),
	  m_direction(dir),
	  m_hint(NONE),
	  m_current_val(0.0f),
	  m_connections(0)
	{
	}
	
	inline void value(float f) { m_current_val = f; control_change_sig.emit(f); }

	void add_child(SharedPtr<ObjectModel> c)    { throw; }
	void remove_child(SharedPtr<ObjectModel> c) { throw; }
	
	void connected_to(SharedPtr<PortModel> p)      { ++m_connections; connection_sig.emit(p); }
	void disconnected_from(SharedPtr<PortModel> p) { --m_connections; disconnection_sig.emit(p); }
	
	Type      m_type;
	Direction m_direction;
	Hint      m_hint;
	float     m_current_val;
	size_t    m_connections;
};

typedef list<SharedPtr<PortModel> > PortModelList;


} // namespace Client
} // namespace Ingen

#endif // PORTMODEL_H
