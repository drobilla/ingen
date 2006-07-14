/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "NoteOffEvent.h"
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "ObjectStore.h"
#include "Node.h"
#include "MidiNoteNode.h"
#include "MidiTriggerNode.h"

namespace Om {


/** Note off with patch explicitly passed - triggered by MIDI.
 */
NoteOffEvent::NoteOffEvent(CountedPtr<Responder> responder, samplecount timestamp, Node* node, uchar note_num)
: Event(responder, timestamp),
  m_node(node),
  m_note_num(note_num)
{
}


/** Note off event with lookup - triggered by OSC.
 */
NoteOffEvent::NoteOffEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& node_path, uchar note_num)
: Event(responder, timestamp),
  m_node(NULL),
  m_node_path(node_path),
  m_note_num(note_num)
{
}


void
NoteOffEvent::execute(samplecount offset)
{	
	if (m_node == NULL && m_node_path != "")
		m_node = om->object_store()->find_node(m_node_path);
		
	// FIXME: this isn't very good at all.
	if (m_node != NULL && m_node->plugin()->type() == Plugin::Internal) {
		if (m_node->plugin()->plug_label() == "note_in")
			((MidiNoteNode*)m_node)->note_off(m_note_num, offset);
		else if (m_node->plugin()->plug_label() == "trigger_in")
			((MidiTriggerNode*)m_node)->note_off(m_note_num, offset);
	}
}


void
NoteOffEvent::post_process()
{
	if (m_node != NULL) 
		_responder->respond_ok();
	else
		_responder->respond_error("Did not find node for note_off");
}


} // namespace Om


