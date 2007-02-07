/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef CREATEPATCHEVENT_H
#define CREATEPATCHEVENT_H

#include "raul/Path.h"
#include "QueuedEvent.h"
#include <string>
using std::string;

template<typename T> class Array;
template<typename T> class TreeNode;

namespace Ingen {
	
class Patch;
class Node;
class Plugin;


/** Creates a new Patch.
 *
 * \ingroup engine
 */
class CreatePatchEvent : public QueuedEvent
{
public:
	CreatePatchEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& path, int poly);

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	enum ErrorType { NO_ERROR, OBJECT_EXISTS, PARENT_NOT_FOUND, INVALID_POLY };
	
	Path              _path;
	Patch*            _patch;
	Patch*            _parent;
	Array<Node*>*     _process_order;
	TreeNode<Node*>*  _patch_treenode;
	int               _poly;
	ErrorType         _error;
};


} // namespace Ingen


#endif // CREATEPATCHEVENT_H
