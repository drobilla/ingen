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

#ifndef DISCONNECTNODEEVENT_H
#define DISCONNECTNODEEVENT_H

#include <string>
#include "raul/List.hpp"
#include "raul/Path.hpp"
#include "QueuedEvent.hpp"

using std::string;

namespace Ingen {

class DisconnectionEvent;
class PatchImpl;
class NodeImpl;
class Connection;
class PortImpl;
class InputPort;
class OutputPort;


/** An event to disconnect all connections to a Node.
 *
 * \ingroup engine
 */
class DisconnectAllEvent : public QueuedEvent
{
public:
	DisconnectAllEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& parent_path, const string& node_path);
	DisconnectAllEvent(Engine& engine, PatchImpl* parent, GraphObjectImpl* object);
	~DisconnectAllEvent();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType { 
		NO_ERROR,
		INVALID_PARENT_PATH,
		PARENT_NOT_FOUND,
		OBJECT_NOT_FOUND,
	};

	Raul::Path                      _parent_path;
	Raul::Path                      _path;
	PatchImpl*                      _parent;
	NodeImpl*                       _node;
	PortImpl*                       _port;
	Raul::List<DisconnectionEvent*> _disconnection_events;
	
	bool _lookup;
	bool _disconnect_parent;

	ErrorType _error;
};


} // namespace Ingen


#endif // DISCONNECTNODEEVENT_H
