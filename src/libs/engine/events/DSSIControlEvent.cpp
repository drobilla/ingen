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

#include "DSSIControlEvent.hpp"
#include "Engine.hpp"
#include "NodeImpl.hpp"
#include "Plugin.hpp"
#include "ObjectStore.hpp"

namespace Ingen {


DSSIControlEvent::DSSIControlEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, int port_num, Sample val)
: QueuedEvent(engine, responder, timestamp),
  _node_path(node_path),
  _port_num(port_num),
  _val(val),
  _node(NULL)
{
}


void
DSSIControlEvent::pre_process()
{
	NodeImpl* node = _engine.object_store()->find_node(_node_path);

	if (node->plugin()->type() != Plugin::DSSI)
		_node = NULL;
	else
		_node = (DSSINode*)node;

	QueuedEvent::pre_process();
}

	
void
DSSIControlEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_node != NULL)
		_node->set_control(_port_num, _val);
}


void
DSSIControlEvent::post_process()
{
	if (_node == NULL)
		std::cerr << "Unable to find DSSI node " << _node_path << std::endl;
}


} // namespace Ingen

