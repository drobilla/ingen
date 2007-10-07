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

#ifndef DISCONNECTPORTEVENT_H
#define DISCONNECTPORTEVENT_H

#include <string>
#include <raul/Path.hpp>
#include "QueuedEvent.hpp"
#include <raul/List.hpp>

namespace Raul { template <typename T> class Array; }

namespace Ingen {

	
class Patch;
class NodeImpl;
class Connection;
class PortImpl;
class DisconnectionEvent;

using std::string;


/** An event to disconnect all connections to a Port.
 *
 * \ingroup engine
 */
class DisconnectPortEvent : public QueuedEvent
{
public:
	DisconnectPortEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& port_path);
	DisconnectPortEvent(Engine& engine, Patch* patch, Port* port);
	~DisconnectPortEvent();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Path                      _port_path;
	Patch*                    _patch;
	Port*                     _port;
	Raul::List<DisconnectionEvent*> _disconnection_events;
	
	Raul::Array<NodeImpl*>* _process_order; // Patch's new process order
	
	bool _succeeded;
	bool _lookup;
};


} // namespace Ingen


#endif // DISCONNECTPORTEVENT_H
