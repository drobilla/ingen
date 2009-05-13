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

#ifndef RENAMEEVENT_H
#define RENAMEEVENT_H

#include "raul/Path.hpp"
#include "QueuedEvent.hpp"
#include "EngineStore.hpp"

namespace Ingen {

class PatchImpl;


/** An event to change the name of an GraphObjectImpl.
 *
 * \ingroup engine
 */
class RenameEvent : public QueuedEvent
{
public:
	RenameEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const Raul::Path& old_path, const Raul::Path& new_path);
	~RenameEvent();

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

#endif // RENAMEEVENT_H
