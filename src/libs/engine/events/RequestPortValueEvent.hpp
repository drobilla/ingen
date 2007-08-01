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

#ifndef REQUESTPORTVALUEEVENT_H
#define REQUESTPORTVALUEEVENT_H

#include <string>
#include "QueuedEvent.hpp"
#include "types.hpp"

using std::string;

namespace Ingen {
	
class Port;
namespace Shared { class ClientInterface; }
using Shared::ClientInterface;


/** A request from a client to send the value of a port.
 *
 * \ingroup engine
 */
class RequestPortValueEvent : public QueuedEvent
{
public:
	RequestPortValueEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& port_path);

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	string           _port_path;
	Port*            _port;
	Sample           _value;
	ClientInterface* _client;
};


} // namespace Ingen

#endif // REQUESTPORTVALUEEVENT_H
