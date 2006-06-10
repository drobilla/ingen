/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef PORTINFO_H
#define PORTINFO_H

#include <cstdlib>
#include <string>
#include <ladspa.h>

using std::string;

namespace Om {

	
enum PortType      { CONTROL, AUDIO, MIDI };
enum PortDirection { INPUT = 0, OUTPUT = 1 }; // FIXME: dupe of ClientInterface::PortDirection
enum PortHint      { NONE, INTEGER, TOGGLE, LOGARITHMIC };


/** Information about a Port.
 *
 * I'm not sure this actually has a reason for existing anymore. This is the
 * model for a Port, but no other OmObjects need a model, soo....
 *
 * \ingroup engine
 */
class PortInfo
{
public:
	PortInfo(const string& port_name, PortType type, PortDirection dir, PortHint hint, float default_val, float min, float max)
	: m_name(port_name),
	  m_type(type),
	  m_direction(dir),
	  m_hint(hint),
	  m_default_val(default_val),
	  m_min_val(min),
	  m_max_val(max)
	{}

	PortInfo(const string& port_name, PortType type, PortDirection dir, float default_val, float min, float max)
	: m_name(port_name),
	  m_type(type),
	  m_direction(dir),
	  m_hint(NONE),
	  m_default_val(default_val),
	  m_min_val(min),
	  m_max_val(max)
	{}

	PortInfo(const string& port_name, PortType type, PortDirection dir, PortHint hint = NONE)
	: m_name(port_name),
	  m_type(type),
	  m_direction(dir),
	  m_hint(hint),
	  m_default_val(0.0f),
	  m_min_val(0.0f),
	  m_max_val(1.0f)
	{}

	PortInfo(const string& port_name, LADSPA_PortDescriptor d, LADSPA_PortRangeHintDescriptor hint)
	: m_name(port_name),
	  m_default_val(1.0f),
	  m_min_val(0.0f),
	  m_max_val(1.0f)
	{
		if (LADSPA_IS_PORT_AUDIO(d)) m_type = AUDIO;
		else if (LADSPA_IS_PORT_CONTROL(d)) m_type = CONTROL;
		else exit(EXIT_FAILURE);
		
		if (LADSPA_IS_PORT_INPUT(d)) m_direction = INPUT;
		else if (LADSPA_IS_PORT_OUTPUT(d)) m_direction = OUTPUT;
		else exit(EXIT_FAILURE);

		if (LADSPA_IS_HINT_TOGGLED(hint)) {
			m_hint = TOGGLE; m_min_val = 0; m_max_val = 1; m_default_val = 0;
		} else if (LADSPA_IS_HINT_LOGARITHMIC(hint))
			m_hint = LOGARITHMIC;
		else if (LADSPA_IS_HINT_INTEGER(hint))
			m_hint = INTEGER;
		else
			m_hint = NONE;
	}

	PortType      type() const         { return m_type; }
	void          type(PortType t)     { m_type = t; }
	float         min_val() const      { return m_min_val; }
	void          min_val(float f)     { m_min_val = f; }
	float         default_val() const  { return m_default_val; }
	void          default_val(float f) { m_default_val = f; }
	float         max_val() const      { return m_max_val; }
	void          max_val(float f)     { m_max_val = f; }
	PortDirection direction() const    { return m_direction; }

	string type_string() {
		switch (m_type) {
			case CONTROL: return "CONTROL"; break;
			case AUDIO:   return "AUDIO"; break;
			case MIDI:    return "MIDI"; break;
			default:      return "UNKNOWN";
		}
	}
	string direction_string() { if (m_direction == INPUT) return "INPUT"; else return "OUTPUT"; }
	string hint_string() { if (m_hint == INTEGER) return "INTEGER";
		else if (m_hint == LOGARITHMIC) return "LOGARITHMIC";
		else if (m_hint == TOGGLE) return "TOGGLE";
		else return "NONE"; }

	bool is_control() const   { return (m_type == CONTROL); }
	bool is_audio() const     { return (m_type == AUDIO); }
	bool is_midi() const      { return (m_type == MIDI); }
	bool is_input() const     { return (m_direction == INPUT); }
	bool is_output() const    { return (m_direction == OUTPUT); }
	
	bool is_logarithmic() const { return (m_hint == LOGARITHMIC); }
	bool is_integer() const     { return (m_hint == INTEGER); }
	bool is_toggle() const      { return (m_hint == TOGGLE); }

	const string& name() const          { return m_name; }
	void          name(const string& n) { m_name = n; }

private:
	// Prevent copies (undefined)
	PortInfo(const PortInfo&);
	PortInfo& operator=(const PortInfo&);
	
	string        m_name;
	PortType      m_type;
	PortDirection m_direction;
	PortHint      m_hint;
	float         m_default_val;
	float         m_min_val;
	float         m_max_val;	
};


} // namespace Om

#endif // PORTINFO_H
