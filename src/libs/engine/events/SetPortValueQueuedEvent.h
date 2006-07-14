/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SETPORTVALUEQUEUEDEVENT_H
#define SETPORTVALUEQUEUEDEVENT_H

#include "QueuedEvent.h"
#include "types.h"
#include <string>
using std::string;

namespace Om {

class Port;


/** An event to change the value of a port.
 *
 * \ingroup engine
 */
class SetPortValueQueuedEvent : public QueuedEvent
{
public:
	SetPortValueQueuedEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& port_path, sample val);
	SetPortValueQueuedEvent(CountedPtr<Responder> responder, samplecount timestamp, size_t voice_num, const string& port_path, sample val);

	void pre_process();
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

#endif // SETPORTVALUEQUEUEDEVENT_H
