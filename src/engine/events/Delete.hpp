/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_DELETE_HPP
#define INGEN_EVENTS_DELETE_HPP

#include "QueuedEvent.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "ControlBindings.hpp"

namespace Raul {
	template<typename T> class Array;
	template<typename T> class ListNode;
}

namespace Ingen {

class GraphObjectImpl;
class NodeImpl;
class PortImpl;
class DriverPort;
class CompiledPatch;

namespace Events {

class DisconnectAll;


/** \page methods
 * <h2>DELETE</h2>
 * As per WebDAV (RFC4918 S9.6).
 *
 * Remove an object from the engine and destroy it.
 *
 * \li All properties of the object are lost
 * \li All references to the object are lost (e.g. the parent's reference to
 *     this child is lost, any connections to the object are removed, etc.)
 */

/** DELETE a graph object (see \ref methods).
 * \ingroup engine
 */
class Delete : public QueuedEvent
{
public:
	Delete(
			Engine&            engine,
			SharedPtr<Request> request,
			FrameTime          timestamp,
			const Raul::Path&  path);

	~Delete();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path                     _path;
	EngineStore::iterator          _store_iterator;
	SharedPtr<NodeImpl>            _node;                ///< Non-NULL iff a node
	SharedPtr<PortImpl>            _port;                ///< Non-NULL iff a port
	Raul::Deletable*               _garbage;
	DriverPort*                    _driver_port;
	PatchImpl::Nodes::Node*        _patch_node_listnode;
	Raul::List<PortImpl*>::Node*   _patch_port_listnode;
	Raul::Array<PortImpl*>*        _ports_array;         ///< New (external) ports for Patch
	CompiledPatch*                 _compiled_patch;      ///< Patch's new process order
	DisconnectAll*                 _disconnect_event;

	SharedPtr<ControlBindings::Bindings> _removed_bindings;

	SharedPtr< Raul::Table<Raul::Path, SharedPtr<Shared::GraphObject> > > _removed_table;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_DELETE_HPP
