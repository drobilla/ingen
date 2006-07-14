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

#ifndef REQUESTPORTVALUEEVENT_H
#define REQUESTPORTVALUEEVENT_H

#include <string>
#include "QueuedEvent.h"
#include "types.h"

using std::string;

namespace Om {
	
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
	RequestPortValueEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& port_path);

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	string                      m_port_path;
	Port*                       m_port;
	sample                      m_value;
	CountedPtr<ClientInterface> m_client;
};


} // namespace Om

#endif // REQUESTPORTVALUEEVENT_H
