/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_DISCONNECTALL_HPP
#define INGEN_EVENTS_DISCONNECTALL_HPP

#include <list>

#include "raul/Path.hpp"

#include "Disconnect.hpp"
#include "QueuedEvent.hpp"

namespace Ingen {
namespace Server {

class CompiledPatch;
class NodeImpl;
class PatchImpl;
class PortImpl;

namespace Events {

class Disconnect;

/** An event to disconnect all connections to a Node.
 *
 * \ingroup engine
 */
class DisconnectAll : public QueuedEvent
{
public:
	DisconnectAll(
			Engine&            engine,
			SharedPtr<Request> request,
			SampleCount        timestamp,
			const Raul::Path&  parent,
			const Raul::Path&  object);

	DisconnectAll(
			Engine&          engine,
			PatchImpl*       parent,
			GraphObjectImpl* object);

	~DisconnectAll();

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

	void maybe_remove_connection(ConnectionImpl* c);

	Raul::Path _parent_path;
	Raul::Path _path;
	PatchImpl* _parent;
	NodeImpl*  _node;
	PortImpl*  _port;

	typedef std::list<Disconnect::Impl*> Impls;
	Impls _impls;

	CompiledPatch* _compiled_patch; ///< New process order for Patch

	bool _deleting;
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_DISCONNECTALL_HPP