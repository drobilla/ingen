/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "AllNotesOffEvent.h"
#include "Responder.h"
#include "Om.h"
#include "OmApp.h"
#include "ObjectStore.h"

namespace Om {


/** Note off with patch explicitly passed - triggered by MIDI.
 */
AllNotesOffEvent::AllNotesOffEvent(CountedPtr<Responder> responder, Patch* patch)
: Event(responder),
  m_patch(patch)
{
}


/** Note off event with lookup - triggered by OSC.
 */
AllNotesOffEvent::AllNotesOffEvent(CountedPtr<Responder> responder, const string& patch_path)
: Event(responder),
  m_patch(NULL),
  m_patch_path(patch_path)
{
}


void
AllNotesOffEvent::execute(samplecount offset)
{	
	if (m_patch == NULL && m_patch_path != "")
		m_patch = om->object_store()->find_patch(m_patch_path);
		
	//if (m_patch != NULL)
	//	for (List<MidiInNode*>::iterator j = m_patch->midi_in_nodes().begin(); j != m_patch->midi_in_nodes().end(); ++j)
	//		(*j)->all_notes_off(offset);
}


void
AllNotesOffEvent::post_process()
{
	if (m_patch != NULL)
		m_responder->respond_ok();
}


} // namespace Om


