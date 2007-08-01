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

#include "NoteOnEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "ObjectStore.hpp"
#include "Node.hpp"
#include "MidiNoteNode.hpp"
#include "MidiTriggerNode.hpp"
#include "Plugin.hpp"

namespace Ingen {


/** Note on with Patch explicitly passed.
 *
 * Used to be triggered by MIDI.  Not used anymore.
 */
NoteOnEvent::NoteOnEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, Node* patch, uchar note_num, uchar velocity)
: Event(engine, responder, timestamp),
  _node(patch),
  _note_num(note_num),
  _velocity(velocity),
  _is_osc_triggered(false)
{
}


/** Note on with Node lookup.
 *
 * Triggered by OSC.
 */
NoteOnEvent::NoteOnEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, uchar note_num, uchar velocity)
: Event(engine, responder, timestamp),
  _node(NULL),
  _node_path(node_path),
  _note_num(note_num),
  _velocity(velocity),
  _is_osc_triggered(true)
{
}


void
NoteOnEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	Event::execute(nframes, start, end);
	assert(_time >= start && _time <= end);

	// Lookup if neccessary
	if (_is_osc_triggered)
		_node = _engine.object_store()->find_node(_node_path);
		
	// FIXME: this isn't very good at all.
	if (_node != NULL && _node->plugin()->type() == Plugin::Internal) {
		if (_node->plugin()->plug_label() == "note_in")
			((MidiNoteNode*)_node)->note_on(_note_num, _velocity, _time, nframes, start, end);
		else if (_node->plugin()->plug_label() == "trigger_in")
			((MidiTriggerNode*)_node)->note_on(_note_num, _velocity, _time, nframes, start, end);
	}
}


void
NoteOnEvent::post_process()
{
	if (_is_osc_triggered) {
		if (_node != NULL)
			_responder->respond_ok();
		else
			_responder->respond_error("Did not find node for note_on");
	}
}


} // namespace Ingen

