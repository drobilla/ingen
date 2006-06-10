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

#ifndef CONNECTIONEVENT_H
#define CONNECTIONEVENT_H

#include <string>
#include "QueuedEvent.h"
#include "util/Path.h"
#include "util/types.h"
using std::string;

template <typename T> class ListNode;
template <typename T> class Array;

namespace Om {
	
class Patch;
class Node;
class Connection;
class MidiMessage;
class Port;
template <typename T> class ConnectionBase;
template <typename T> class InputPort;
template <typename T> class OutputPort;
template <typename T> class TypedConnectionEvent; // helper, defined below


/** Make a Connection between two Ports.
 *
 * \ingroup engine
 */
class ConnectionEvent : public QueuedEvent
{
public:
	ConnectionEvent(CountedPtr<Responder> responder, const string& src_port_path, const string& dst_port_path);
	~ConnectionEvent();

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	
	enum ErrorType { NO_ERROR, PARENT_PATCH_DIFFERENT, PORT_NOT_FOUND, TYPE_MISMATCH };
	
	Path           m_src_port_path;
	Path           m_dst_port_path;
	
	Patch*         m_patch;
	Port*          m_src_port;
	Port*          m_dst_port;

	QueuedEvent*     m_typed_event;
	
	ErrorType m_error;
};


/** Templated ConnectionEvent.
 *
 * Intended to be called from ConnectionEvent so callers (ie OSCReceiver)
 * can use ConnectionEvent without knowing anything about types (which
 * they can't, since all they have is Port paths).
 */
template <typename T>
class TypedConnectionEvent : public QueuedEvent
{
public:
	TypedConnectionEvent(CountedPtr<Responder> responder, OutputPort<T>* src_port, InputPort<T>* dst_port);
	~TypedConnectionEvent();
	
	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	OutputPort<T>*                 m_src_port;
	InputPort<T>*                  m_dst_port;

	Patch*                         m_patch;
	Array<Node*>*                  m_process_order; ///< New process order for Patch
	ConnectionBase<T>*             m_connection;
	ListNode<Connection*>*         m_patch_listnode;
	ListNode<ConnectionBase<T>*>*  m_port_listnode;
	
	bool m_succeeded;
};



} // namespace Om

#endif // CONNECTIONEVENT_H
