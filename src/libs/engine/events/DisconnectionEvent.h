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

#ifndef DISCONNECTIONEVENT_H
#define DISCONNECTIONEVENT_H

#include <string>
#include "util/Path.h"
#include "QueuedEvent.h"
#include "types.h"
using std::string;

template <typename T> class ListNode;
template <typename T> class Array;

namespace Om {
	
class Patch;
class Node;
class Connection;
class MidiMessage;
class Port;
template <typename T> class TypedConnection;
template <typename T> class InputPort;
template <typename T> class OutputPort;
template <typename T> class TypedDisconnectionEvent; // helper, defined below


/** Make a Connection between two Ports.
 *
 * \ingroup engine
 */
class DisconnectionEvent : public QueuedEvent
{
public:
	DisconnectionEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path);
	DisconnectionEvent(CountedPtr<Responder> responder, SampleCount timestamp, Port* const src_port, Port* const dst_port);
	~DisconnectionEvent();

	void pre_process();
	void execute(SampleCount offset);
	void post_process();

private:
	
	enum ErrorType { NO_ERROR, PARENT_PATCH_DIFFERENT, PORT_NOT_FOUND, TYPE_MISMATCH };
	
	Path           m_src_port_path;
	Path           m_dst_port_path;
	
	Patch*         m_patch;
	Port*          m_src_port;
	Port*          m_dst_port;

	bool           m_lookup;
	QueuedEvent*     m_typed_event;
	
	ErrorType m_error;
};


/** Templated DisconnectionEvent.
 *
 * Intended to be called from DisconnectionEvent so callers (ie OSCReceiver)
 * can use DisconnectionEvent without knowing anything about types (which
 * they can't, since all they have is Port paths).
 */
template <typename T>
class TypedDisconnectionEvent : public QueuedEvent
{
public:
	TypedDisconnectionEvent(CountedPtr<Responder> responder, SampleCount timestamp, OutputPort<T>* src_port, InputPort<T>* dst_port);
	
	void pre_process();
	void execute(SampleCount offset);
	void post_process();

private:
	OutputPort<T>*                 m_src_port;
	InputPort<T>*                  m_dst_port;

	Patch*                         m_patch;
	Array<Node*>*                  m_process_order; ///< New process order for Patch
	
	bool m_succeeded;
};



} // namespace Om

#endif // DISCONNECTIONEVENT_H
