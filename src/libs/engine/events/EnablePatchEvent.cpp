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

#include "EnablePatchEvent.h"
#include "Responder.h"
#include "Engine.h"
#include "Patch.h"
#include "util.h"
#include "ClientBroadcaster.h"
#include "ObjectStore.h"

namespace Ingen {


EnablePatchEvent::EnablePatchEvent(CountedPtr<Responder> responder, SampleCount timestamp, const string& patch_path)
: QueuedEvent(responder, timestamp),
  m_patch_path(patch_path),
  m_patch(NULL),
  m_process_order(NULL)
{
}


void
EnablePatchEvent::pre_process()
{
	m_patch = Engine::instance().object_store()->find_patch(m_patch_path);
	
	if (m_patch != NULL) {
		/* Any event that requires a new process order will set the patch's
		 * process order to NULL if it is executed when the patch is not
		 * active.  So, if the PO is NULL, calculate it here */
		if (m_patch->process_order() == NULL)
			m_process_order = m_patch->build_process_order();
	}

	QueuedEvent::pre_process();
}


void
EnablePatchEvent::execute(SampleCount offset)
{
	if (m_patch != NULL) {
		m_patch->process(true);

		if (m_patch->process_order() == NULL)
			m_patch->process_order(m_process_order);
	}
	
	QueuedEvent::execute(offset);
}


void
EnablePatchEvent::post_process()
{
	if (m_patch != NULL) {
		_responder->respond_ok();
		Engine::instance().client_broadcaster()->send_patch_enable(m_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + m_patch_path + " not found");
	}
}


} // namespace Ingen

