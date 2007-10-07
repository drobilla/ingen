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

#include <string>
#include <raul/Path.hpp>
#include "QueuedEvent.hpp"
#include "ObjectStore.hpp"

using std::string;

template<typename T> class TreeNode;
template<typename T> class ListNode;

namespace Ingen {

class GraphObjectImpl;
class Patch;
class NodeImpl;
class Plugin;
class DisconnectNodeEvent;
class DisconnectPortEvent;


/** An event to change the name of an GraphObjectImpl.
 *
 * \ingroup engine
 */
class RenameEvent : public QueuedEvent
{
public:
	RenameEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, const string& name);
	~RenameEvent();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum ErrorType { NO_ERROR, OBJECT_NOT_FOUND, OBJECT_EXISTS, OBJECT_NOT_RENAMABLE, INVALID_NAME };

	Path                           _old_path;
	string                         _name;
	Path                           _new_path;
	Patch*                         _parent_patch;
	ObjectStore::Objects::iterator _store_iterator;
	ErrorType                      _error;
};


} // namespace Ingen

#endif // RENAMEEVENT_H
