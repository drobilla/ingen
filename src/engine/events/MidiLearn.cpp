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

#include "events/MidiLearn.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "internals/Controller.hpp"
#include "ClientBroadcaster.hpp"
#include "PluginImpl.hpp"

using namespace std;

namespace Ingen {
namespace Events {


MidiLearn::MidiLearn(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const Raul::Path& node_path)
	: QueuedEvent(engine, responder, timestamp)
	, _error(NO_ERROR)
	, _node_path(node_path)
	, _node(NULL)
{
}


void
MidiLearn::pre_process()
{
	_node = _engine.engine_store()->find_node(_node_path);

	QueuedEvent::pre_process();
}


void
MidiLearn::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_node != NULL) {
	   if (_node->plugin_impl()->type() == Shared::Plugin::Internal) {
		   ((NodeBase*)_node)->learn();
	   } else {
		   _error = INVALID_NODE_TYPE;
	   }
	}
}


void
MidiLearn::post_process()
{
	if (_error == NO_ERROR) {
		_responder->respond_ok();
	} else if (_node == NULL) {
		string msg = "Did not find node '";
		msg.append(_node_path.str()).append("' for MIDI learn.");
		_responder->respond_error(msg);
	} else {
		const string msg = string("Node '") + _node_path.str() + "' is not capable of MIDI learn.";
		_responder->respond_error(msg);
	}
}


} // namespace Ingen
} // namespace Events

