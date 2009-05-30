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

#ifndef CLEARPATCHEVENT_H
#define CLEARPATCHEVENT_H

#include "raul/Array.hpp"
#include "raul/Table.hpp"
#include "raul/Path.hpp"
#include "QueuedEvent.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"

namespace Ingen {

class PatchImpl;
class DriverPort;


/** Delete all nodes from a patch.
 *
 * \ingroup engine
 */
class ClearPatchEvent : public QueuedEvent
{
public:
	ClearPatchEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const Raul::Path& patch_path);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	const Raul::Path        _patch_path;
	SharedPtr<PatchImpl>    _patch;
	bool                    _process;
	Raul::Array<PortImpl*>* _ports_array;    ///< New (external) ports for Patch
	CompiledPatch*          _compiled_patch; ///< Patch's new process order

	typedef Raul::Array<Raul::List<DriverPort*>::Node*> DriverPorts;
	DriverPorts* _driver_ports;

	SharedPtr<EngineStore::Objects> _removed_table;
};


} // namespace Ingen


#endif // CLEARPATCHEVENT_H
