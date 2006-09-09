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

#include "AllNotesOffEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "ObjectStore.h"

namespace Ingen {


/** Note off with patch explicitly passed - triggered by MIDI.
 */
AllNotesOffEvent::AllNotesOffEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, Patch* patch)
: Event(engine, responder, timestamp),
  m_patch(patch)
{
}


/** Note off event with lookup - triggered by OSC.
 */
AllNotesOffEvent::AllNotesOffEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& patch_path)
: Event(engine, responder, timestamp),
  m_patch(NULL),
  m_patch_path(patch_path)
{
}


void
AllNotesOffEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	Event::execute(nframes, start, end);

	if (m_patch == NULL && m_patch_path != "")
		m_patch = _engine.object_store()->find_patch(m_patch_path);
		
	//if (m_patch != NULL)
	//	for (List<MidiInNode*>::iterator j = m_patch->midi_in_nodes().begin(); j != m_patch->midi_in_nodes().end(); ++j)
	//		(*j)->all_notes_off(offset);
}


void
AllNotesOffEvent::post_process()
{
	if (m_patch != NULL)
		_responder->respond_ok();
}


} // namespace Ingen


