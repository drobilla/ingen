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

#include "QueuedEvent.hpp"
#include "types.hpp"
#include <string>
using std::string;

namespace Ingen {

class PortImpl;


/** An event to change the value of a port.
 *
 * \ingroup engine
 */
class SetPortValueQueuedEvent : public QueuedEvent
{
public:
	SetPortValueQueuedEvent(Engine&              engine,
	                        SharedPtr<Responder> responder,
	                        SampleCount          timestamp,
	                        const string&        port_path,
	                        const string&        data_type,
	                        uint32_t             data_size,
	                        const void*          data);
	
	SetPortValueQueuedEvent(Engine&              engine,
	                        SharedPtr<Responder> responder,
	                        SampleCount          timestamp,
	                        uint32_t             voice_num,
	                        const string&        port_path,
	                        const string&        data_type,
	                        uint32_t             data_size,
	                        const void*          data);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType { NO_ERROR, PORT_NOT_FOUND, NO_SPACE };
	
	bool         _omni;
	uint32_t     _voice_num;
	string       _port_path;
	const string _data_type;
	uint32_t     _data_size;
	void*        _data;
	PortImpl*    _port;
	ErrorType    _error;
};


} // namespace Ingen

#endif // SETPORTVALUEQUEUEDEVENT_H
