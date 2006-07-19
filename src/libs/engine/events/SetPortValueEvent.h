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

#ifndef SETPORTVALUEEVENT_H
#define SETPORTVALUEEVENT_H

#include <string>
#include "Event.h"
#include "types.h"
using std::string;

namespace Ingen {

class Port;


/** An event to change the value of a port.
 *
 * \ingroup engine
 */
class SetPortValueEvent : public Event
{
public:
	SetPortValueEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& port_path, Sample val);
	SetPortValueEvent(CountedPtr<Responder> responder, SampleCount timestamp, size_t voice_num, const string& port_path, Sample val);

	void execute(SampleCount offset);
	void post_process();

private:
	enum ErrorType { NO_ERROR, PORT_NOT_FOUND, TYPE_MISMATCH };
	
	int       m_voice_num;
	string    m_port_path;
	float     m_val;
	Port*     m_port;
	ErrorType m_error;
};


} // namespace Ingen

#endif // SETPORTVALUEEVENT_H
