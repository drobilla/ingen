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

#ifndef ADDNODEEVENT_H
#define ADDNODEEVENT_H

#include "QueuedEvent.hpp"
#include <raul/Path.hpp>
#include <string>
using std::string;

namespace Raul { template <typename T> class Array; }
template<typename T> class TreeNode;

namespace Ingen {

class Patch;
class Node;
class Plugin;
class CompiledPatch;


/** An event to load a Node and insert it into a Patch.
 *
 * \ingroup engine
 */
class AddNodeEvent : public QueuedEvent
{
public:
	AddNodeEvent(Engine&               engine,
	             SharedPtr<Responder> responder,
	             SampleCount           timestamp,
	             const string&         node_path,
	             const string&         plugin_uri,
	             bool                  poly);
	
	// DEPRECATED
	AddNodeEvent(Engine&               engine,
	             SharedPtr<Responder> responder,
	             SampleCount           timestamp,
	             const string&         node_path,
				 const string&         plugin_type,
				 const string&         lib_name,
	             const string&         plugin_label,
	             bool                  poly);

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	string         _patch_name;
	Raul::Path     _path;
	string         _plugin_uri; ///< If nonempty then type, library, label, are ignored
	string         _plugin_type;
	string         _plugin_lib;
	string         _plugin_label;
	bool           _poly;
	Patch*         _patch;
	Node*          _node;
	CompiledPatch* _compiled_patch; ///< Patch's new process order
	bool           _node_already_exists;
};


} // namespace Ingen

#endif // ADDNODEEVENT_H
