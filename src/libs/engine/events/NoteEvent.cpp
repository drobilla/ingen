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

#include "NoteEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "ObjectStore.hpp"
#include "Node.hpp"
#include "MidiNoteNode.hpp"
#include "MidiTriggerNode.hpp"
#include "Plugin.hpp"
#include "ProcessContext.hpp"

namespace Ingen {


/** Note on with Patch explicitly passed.
 *
 * Used to be triggered by MIDI.  Not used anymore.
 */
NoteEvent::NoteEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, Node* node, bool on, uchar note_num, uchar velocity)
: Event(engine, responder, timestamp),
  _node(node),
  _on(on),
  _note_num(note_num),
  _velocity(velocity)
{
}


/** Note on with Node lookup.
 *
 * Triggered by OSC.
 */
NoteEvent::NoteEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, bool on, uchar note_num, uchar velocity)
: Event(engine, responder, timestamp),
  _node(NULL),
  _node_path(node_path),
  _on(on),
  _note_num(note_num),
  _velocity(velocity)
{
}


void
NoteEvent::execute(ProcessContext& context)
{
	Event::execute(context);
	assert(_time >= context.start() && _time <= context.end());

	// Lookup if neccessary
	if (!_node)
		_node = _engine.object_store()->find_node(_node_path);
		
	// FIXME: barf
	
	if (_node != NULL && _node->plugin()->type() == Plugin::Internal) {
		if (_on) {
			if (_node->plugin()->plug_label() == "note_in")
				((MidiNoteNode*)_node)->note_on(_note_num, _velocity, _time, context);
			else if (_node->plugin()->plug_label() == "trigger_in")
				((MidiTriggerNode*)_node)->note_on(_note_num, _velocity, _time, context);
		} else  {
			if (_node->plugin()->plug_label() == "note_in")
				((MidiNoteNode*)_node)->note_off(_note_num, _time, context);
			else if (_node->plugin()->plug_label() == "trigger_in")
				((MidiTriggerNode*)_node)->note_off(_note_num, _time, context);
		}
	}
}


void
NoteEvent::post_process()
{
	if (_responder) {
		if (_node)
			_responder->respond_ok();
		else
			_responder->respond_error("Did not find node for note_on");
	}
}


} // namespace Ingen

