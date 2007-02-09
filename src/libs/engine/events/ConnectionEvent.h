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

#ifndef CONNECTIONEVENT_H
#define CONNECTIONEVENT_H

#include <string>
#include "QueuedEvent.h"
#include "raul/Path.h"
#include "types.h"
using std::string;

namespace Raul {
	template <typename T> class ListNode;
	template <typename T> class Array;
}

namespace Ingen {
	
class Patch;
class Node;
class Connection;
class MidiMessage;
class Port;
template <typename T> class TypedConnection;
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
	ConnectionEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& src_port_path, const string& dst_port_path);
	~ConnectionEvent();

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	
	enum ErrorType { NO_ERROR, PARENT_PATCH_DIFFERENT, PORT_NOT_FOUND, TYPE_MISMATCH };
	
	Raul::Path _src_port_path;
	Raul::Path _dst_port_path;
	
	Patch* _patch;
	Port*  _src_port;
	Port*  _dst_port;

	QueuedEvent* _typed_event;
	
	ErrorType _error;
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
	TypedConnectionEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, OutputPort<T>* src_port, InputPort<T>* dst_port);
	
	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	OutputPort<T>*                 _src_port;
	InputPort<T>*                  _dst_port;

	Patch*                         _patch;
	Raul::Array<Node*>*                  _process_order; ///< New process order for Patch
	TypedConnection<T>*            _connection;
	Raul::ListNode<Connection*>*         _patch_listnode;
	Raul::ListNode<TypedConnection<T>*>* _port_listnode;
	
	bool _succeeded;
};



} // namespace Ingen

#endif // CONNECTIONEVENT_H
