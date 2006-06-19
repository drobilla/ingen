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

#ifndef DISCONNECTNODEEVENT_H
#define DISCONNECTNODEEVENT_H

#include <string>
#include "util/Path.h"
#include "QueuedEvent.h"
#include "List.h"
using std::string;

namespace Om {

class DisconnectionEvent;
class Patch;
class Node;
class Connection;
template <typename T> class TypedConnection;
class Port;
template <typename T> class InputPort;
template <typename T> class OutputPort;


/** An event to disconnect all connections to a Node.
 *
 * \ingroup engine
 */
class DisconnectNodeEvent : public QueuedEvent
{
public:
	DisconnectNodeEvent(CountedPtr<Responder> responder, const string& node_path);
	DisconnectNodeEvent(Node* node);
	~DisconnectNodeEvent();

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	Path                      m_node_path;
	Patch*                    m_patch;
	Node*                     m_node;
	List<DisconnectionEvent*> m_disconnection_events;
	
	bool m_succeeded;
	bool m_lookup;
	bool m_disconnect_parent;
};


} // namespace Om


#endif // DISCONNECTNODEEVENT_H
