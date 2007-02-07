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

#ifndef SETPORTVALUEQUEUEDEVENT_H
#define SETPORTVALUEQUEUEDEVENT_H

#include "QueuedEvent.h"
#include "types.h"
#include <string>
using std::string;

namespace Ingen {

class Port;


/** An event to change the value of a port.
 *
 * \ingroup engine
 */
class SetPortValueQueuedEvent : public QueuedEvent
{
public:
	SetPortValueQueuedEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path, Sample val);
	SetPortValueQueuedEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, size_t voice_num, const string& port_path, Sample val);

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	enum ErrorType { NO_ERROR, PORT_NOT_FOUND, TYPE_MISMATCH };
	
	int       _voice_num;
	string    _port_path;
	float     _val;
	Port*     _port;
	ErrorType _error;
};


} // namespace Ingen

#endif // SETPORTVALUEQUEUEDEVENT_H
