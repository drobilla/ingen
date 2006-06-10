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

#ifndef SETPORTVALUEEVENT_H
#define SETPORTVALUEEVENT_H

#include <string>
#include "Event.h"
#include "util/types.h"
using std::string;

namespace Om {

class Port;


/** An event to change the value of a port.
 *
 * \ingroup engine
 */
class SetPortValueEvent : public Event
{
public:
	SetPortValueEvent(CountedPtr<Responder> responder, const string& port_path, sample val);
	SetPortValueEvent(CountedPtr<Responder> responder, size_t voice_num, const string& port_path, sample val);

	void execute(samplecount offset);
	void post_process();

private:
	enum ErrorType { NO_ERROR, PORT_NOT_FOUND, TYPE_MISMATCH };
	
	int       m_voice_num;
	string    m_port_path;
	float     m_val;
	Port*     m_port;
	ErrorType m_error;
};


} // namespace Om

#endif // SETPORTVALUEEVENT_H
