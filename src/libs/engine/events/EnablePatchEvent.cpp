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

#include "EnablePatchEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "PatchImpl.hpp"
#include "util.hpp"
#include "ClientBroadcaster.hpp"
#include "EngineStore.hpp"

namespace Ingen {


EnablePatchEvent::EnablePatchEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& patch_path, bool enable)
: QueuedEvent(engine, responder, timestamp),
  _patch_path(patch_path),
  _patch(NULL),
  _compiled_patch(NULL),
  _enable(enable)
{
}


void
EnablePatchEvent::pre_process()
{
	_patch = _engine.object_store()->find_patch(_patch_path);
	
	if (_enable && _patch) {
		/* Any event that requires a new process order will set the patch's
		 * compiled_patch to NULL if it is executed when the patch is not
		 * active.  So, if the CP is NULL, calculate it here */
		if (_patch->compiled_patch() == NULL)
			_compiled_patch = _patch->compile();
	}

	QueuedEvent::pre_process();
}


void
EnablePatchEvent::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch != NULL) {
		if (_enable)
			_patch->enable();
		else
			_patch->disable();

		if (_enable && _patch->compiled_patch() == NULL)
			_patch->compiled_patch(_compiled_patch);
	}
}


void
EnablePatchEvent::post_process()
{
	if (_patch != NULL) {
		_responder->respond_ok();
		if (_enable)
			_engine.broadcaster()->send_patch_enable(_patch_path);
		else
			_engine.broadcaster()->send_patch_disable(_patch_path);
	} else {
		_responder->respond_error(string("Patch ") + _patch_path + " not found");
	}
}


} // namespace Ingen

