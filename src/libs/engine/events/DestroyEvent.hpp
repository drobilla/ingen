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

#ifndef DESTROYEVENT_H
#define DESTROYEVENT_H

#include <string>
#include <raul/Path.hpp>
#include "QueuedEvent.hpp"


using std::string;

namespace Raul {
	template<typename T> class Array;
	template<typename T> class ListNode;
}
template<typename T> class TreeNode;

namespace Ingen {

class GraphObject;
class Patch;
class Node;
class Port;
class DriverPort;
class Plugin;
class DisconnectNodeEvent;
class DisconnectPortEvent;


/** An event to remove and delete a Node.
 *
 * \ingroup engine
 */
class DestroyEvent : public QueuedEvent
{
public:
	DestroyEvent(Engine& engine, SharedPtr<Shared::Responder> responder, FrameTime timestamp, QueuedEventSource* source, const string& path, bool block = true);
	DestroyEvent(Engine& engine, SharedPtr<Shared::Responder> responder, FrameTime timestamp, QueuedEventSource* source, Node* node, bool block = true);
	~DestroyEvent();

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	Path                    _path;
	GraphObject*            _object;
	Node*                   _node;  ///< Same as _object if it is a Node, otherwise NULL
	Port*                   _port;  ///< Same as _object if it is a Port, otherwise NULL
	DriverPort*             _driver_port;
	Raul::ListNode<Node*>*        _patch_node_listnode;
	Raul::ListNode<Port*>*        _patch_port_listnode;
	TreeNode<GraphObject*>* _store_treenode;
	Raul::Array<Port*>*           _ports_array; ///< New (external) ports array for Patch
	Raul::Array<Node*>*           _process_order;  ///< Patch's new process order
	DisconnectNodeEvent*    _disconnect_node_event;
	DisconnectPortEvent*    _disconnect_port_event;
};


} // namespace Ingen

#endif // DESTROYEVENT_H
