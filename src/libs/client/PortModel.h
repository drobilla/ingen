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

namespace LibOmClient {

/* Model of a port.
 *
 * \ingroup libomclient.
 */
class PortModel : public ObjectModel
{
public:
	enum Type      { CONTROL, AUDIO, MIDI };
	enum Direction { INPUT, OUTPUT };
	enum Hint      { NONE, INTEGER, TOGGLE, LOGARITHMIC };
	
	PortModel(const string& path, Type type, Direction dir, Hint hint,
		float default_val, float min, float max)
	: ObjectModel(path),
	  m_type(type),
	  m_direction(dir),
	  m_hint(hint),
	  m_default_val(default_val),
	  m_min_val(min),
	  m_user_min(min),
	  m_max_val(max),
	  m_user_max(max),
	  m_current_val(default_val),
	  m_connected(false)
	{
	}
	
	PortModel(const string& path, Type type, Direction dir)
	: ObjectModel(path),
	  m_type(type),
	  m_direction(dir),
	  m_hint(NONE),
	  m_default_val(0.0f),
	  m_min_val(0.0f),
	  m_user_min(0.0f),
	  m_max_val(0.0f),
	  m_user_max(0.0f),
	  m_current_val(0.0f),
	  m_connected(false)
	{
	}
	
	inline float min_val() const      { return m_min_val; }
	inline float user_min() const     { return m_user_min; }
	inline void  user_min(float f)    { m_user_min = f; }
	inline float default_val() const  { return m_default_val; }
	inline void  default_val(float f) { m_default_val = f; }
	inline float max_val() const      { return m_max_val; }
	inline float user_max() const     { return m_user_max; }
	inline void  user_max(float f)    { m_user_max = f; }
	inline float value() const        { return m_current_val; }
	inline void  value(float f)       { m_current_val = f; control_change_sig.emit(f); }
	inline bool  connected()          { return m_connected; }
	inline void  connected(bool b)    { m_connected = b; }
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
		{ return (m_path == pm.m_path); }

	// Signals
	sigc::signal<void, float> control_change_sig; ///< "Control" ports only

private:
	// Prevent copies (undefined)
	PortModel(const PortModel& copy);
	PortModel& operator=(const PortModel& copy);
	
	Type      m_type;
	Direction m_direction;
	Hint      m_hint;
	float     m_default_val;
	float     m_min_val;
	float     m_user_min;
	float     m_max_val;
	float     m_user_max;
	float     m_current_val;
	bool      m_connected;
};

typedef list<CountedPtr<PortModel> > PortModelList;


} // namespace LibOmClient

#endif // PORTMODEL_H
