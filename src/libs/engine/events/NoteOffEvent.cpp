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

#include "NoteOffEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "ObjectStore.hpp"
#include "Node.hpp"
#include "Plugin.hpp"
#include "MidiNoteNode.hpp"
#include "MidiTriggerNode.hpp"
#include "ProcessContext.hpp"

namespace Ingen {


/** Note off with patch explicitly passed - triggered by MIDI.
 */
NoteOffEvent::NoteOffEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, Node* node, uchar note_num)
: Event(engine, responder, timestamp),
  _node(node),
  _note_num(note_num)
{
}


/** Note off event with lookup - triggered by OSC.
 */
NoteOffEvent::NoteOffEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, uchar note_num)
: Event(engine, responder, timestamp),
  _node(NULL),
  _node_path(node_path),
  _note_num(note_num)
{
}


void
NoteOffEvent::execute(ProcessContext& context)
{	
	Event::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	if (_node == NULL && _node_path != "")
		_node = _engine.object_store()->find_node(_node_path);
		
	// FIXME: this isn't very good at all.
	if (_node != NULL && _node->plugin()->type() == Plugin::Internal) {
		if (_node->plugin()->plug_label() == "note_in")
			((MidiNoteNode*)_node)->note_off(_note_num, _time, context);
		else if (_node->plugin()->plug_label() == "trigger_in")
			((MidiTriggerNode*)_node)->note_off(_note_num, _time, context);
	}
}


void
NoteOffEvent::post_process()
{
	if (_node != NULL) 
		_responder->respond_ok();
	else
		_responder->respond_error("Did not find node for note_off");
}


} // namespace Ingen


