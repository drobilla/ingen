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

#ifndef ENABLEPORTBROADCASTINGEVENT_H
#define ENABLEPORTBROADCASTINGEVENT_H

#include <string>
#include "QueuedEvent.hpp"
#include "types.hpp"

using std::string;

namespace Ingen {
	
class PortImpl;
namespace Shared { class ClientInterface; }
using Shared::ClientInterface;


/** Enable sending of dynamic value change notifications for a port.
 *
 * \ingroup engine
 */
class EnablePortBroadcastingEvent : public QueuedEvent
{
public:
	EnablePortBroadcastingEvent(Engine&              engine,
	                            SharedPtr<Responder> responder,
	                            SampleCount          timestamp,
	                            const std::string&   port_path,
								bool                 enable);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	const std::string _port_path;
	PortImpl*         _port;
	bool              _enable;
};


} // namespace Ingen

#endif // ENABLEPORTBROADCASTINGEVENT_H
