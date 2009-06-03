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

#ifndef RENAMEEVENT_H
#define RENAMEEVENT_H

#include "raul/Path.hpp"
#include "QueuedEvent.hpp"
#include "EngineStore.hpp"

namespace Ingen {

class PatchImpl;

namespace Events {


/** \page methods
 * <h2>MOVE</h2>
 * As per WebDAV (RFC4918 S9.9).
 *
 * Move an object from its current location and insert it at a new location
 * in a single operation.
 *
 * MOVE to a path with a different parent is currently not supported.
 */

/** MOVE a graph object to a new path (see \ref methods).
 * \ingroup engine
 */
class Move : public QueuedEvent
{
public:
	Move(
			Engine&              engine,
			SharedPtr<Responder> responder,
			SampleCount          timestamp,
			const Raul::Path&    old_path,
			const Raul::Path&    new_path);
	~Move();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType {
		NO_ERROR,
		OBJECT_NOT_FOUND,
		OBJECT_EXISTS,
		OBJECT_NOT_RENAMABLE,
		PARENT_DIFFERS
	};

	Raul::Path            _old_path;
	Raul::Path            _new_path;
	PatchImpl*            _parent_patch;
	EngineStore::iterator _store_iterator;
	ErrorType             _error;
};


} // namespace Ingen
} // namespace Events

#endif // RENAMEEVENT_H
