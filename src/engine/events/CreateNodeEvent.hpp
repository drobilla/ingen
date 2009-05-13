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

#ifndef CREATENODEEVENT_H
#define CREATENODEEVENT_H

#include <string>
#include "QueuedEvent.hpp"

namespace Ingen {

class PatchImpl;
class NodeImpl;
class CompiledPatch;


/** An event to load a Node and insert it into a Patch.
 *
 * \ingroup engine
 */
class CreateNodeEvent : public QueuedEvent
{
public:
	CreateNodeEvent(Engine&               engine,
	                 SharedPtr<Responder> responder,
	                 SampleCount          timestamp,
	                 const Raul::Path&    node_path,
	                 const Raul::URI&     plugin_uri,
	                 bool                 poly);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path     _path;
	Raul::URI      _plugin_uri; ///< If nonempty then type, library, label, are ignored
	std::string    _plugin_type;
	std::string    _plugin_lib;
	std::string    _plugin_label;
	bool           _polyphonic;
	PatchImpl*     _patch;
	NodeImpl*      _node;
	CompiledPatch* _compiled_patch; ///< Patch's new process order
	bool           _node_already_exists;
};


} // namespace Ingen

#endif // CREATENODEEVENT_H
