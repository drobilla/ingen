/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "QueuedEvent.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"

namespace Raul {
	template<typename T> class Array;
	template<typename T> class ListNode;
}

namespace Ingen {

class GraphObjectImpl;
class NodeImpl;
class PortImpl;
class DriverPort;
class DisconnectAllEvent;
class CompiledPatch;


/** Delete a graph object.
 * WebDAV method DELETE (RFC4918 S9.6).
 *
 * \ingroup engine
 */
class DeleteEvent : public QueuedEvent
{
public:
	DeleteEvent(
			Engine&              engine,
			SharedPtr<Responder> responder,
			FrameTime            timestamp,
			QueuedEventSource*   source,
			const Raul::Path&    path);

	~DeleteEvent();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path                     _path;
	EngineStore::iterator          _store_iterator;
	SharedPtr<NodeImpl>            _node;                ///< Non-NULL iff a node
	SharedPtr<PortImpl>            _port;                ///< Non-NULL iff a port
	Raul::List<DriverPort*>::Node* _driver_port;
	PatchImpl::Nodes::Node*        _patch_node_listnode;
	Raul::List<PortImpl*>::Node*   _patch_port_listnode;
	Raul::Array<PortImpl*>*        _ports_array;         ///< New (external) ports for Patch
	CompiledPatch*                 _compiled_patch;      ///< Patch's new process order
	DisconnectAllEvent*            _disconnect_event;

	SharedPtr< Raul::Table<Raul::Path, SharedPtr<Shared::GraphObject> > > _removed_table;
};


} // namespace Ingen

#endif // DESTROYEVENT_H
