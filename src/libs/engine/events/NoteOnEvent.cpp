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

#include "NoteOnEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "ObjectStore.h"
#include "Node.h"
#include "MidiNoteNode.h"
#include "MidiTriggerNode.h"
#include "Plugin.h"

namespace Ingen {


/** Note on with Patch explicitly passed.
 *
 * Used to be triggered by MIDI.  Not used anymore.
 */
NoteOnEvent::NoteOnEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, Node* patch, uchar note_num, uchar velocity)
: Event(engine, responder, timestamp),
  m_node(patch),
  m_note_num(note_num),
  m_velocity(velocity),
  m_is_osc_triggered(false)
{
}


/** Note on with Node lookup.
 *
 * Triggered by OSC.
 */
NoteOnEvent::NoteOnEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& node_path, uchar note_num, uchar velocity)
: Event(engine, responder, timestamp),
  m_node(NULL),
  m_node_path(node_path),
  m_note_num(note_num),
  m_velocity(velocity),
  m_is_osc_triggered(true)
{
}


void
NoteOnEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	Event::execute(nframes, start, end);
	assert(_time >= start && _time <= end);

	// Lookup if neccessary
	if (m_is_osc_triggered)
		m_node = _engine.object_store()->find_node(m_node_path);
		
	// FIXME: this isn't very good at all.
	if (m_node != NULL && m_node->plugin()->type() == Plugin::Internal) {
		if (m_node->plugin()->plug_label() == "note_in")
			((MidiNoteNode*)m_node)->note_on(m_note_num, m_velocity, _time, nframes, start, end);
		else if (m_node->plugin()->plug_label() == "trigger_in")
			((MidiTriggerNode*)m_node)->note_on(m_note_num, m_velocity, _time, nframes, start, end);
	}
}


void
NoteOnEvent::post_process()
{
	if (m_is_osc_triggered) {
		if (m_node != NULL)
			_responder->respond_ok();
		else
			_responder->respond_error("Did not find node for note_on");
	}
}


} // namespace Ingen

