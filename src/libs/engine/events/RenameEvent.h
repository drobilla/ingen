/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RENAMEEVENT_H
#define RENAMEEVENT_H

#include "QueuedEvent.h"
#include "util/Path.h"
#include <string>
using std::string;

template<typename T> class TreeNode;
template<typename T> class ListNode;

namespace Om {

class GraphObject;
class Patch;
class Node;
class Plugin;
class DisconnectNodeEvent;
class DisconnectPortEvent;


/** An event to change the name of an GraphObject.
 *
 * \ingroup engine
 */
class RenameEvent : public QueuedEvent
{
public:
	RenameEvent(CountedPtr<Responder> responder, const string& path, const string& name);
	~RenameEvent();

	void pre_process();
	void execute(samplecount offset);
	void post_process();

private:
	enum ErrorType { NO_ERROR, OBJECT_NOT_FOUND, OBJECT_EXISTS, OBJECT_NOT_RENAMABLE, INVALID_NAME };

	Path                 m_old_path;
	string               m_name;
	Path                 m_new_path;
	Patch*               m_parent_patch;
	TreeNode<GraphObject*>* m_store_treenode;
	ErrorType            m_error;
};


} // namespace Om

#endif // RENAMEEVENT_H
