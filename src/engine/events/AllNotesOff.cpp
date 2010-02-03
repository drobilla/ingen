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

#include "AllNotesOff.hpp"
#include "Request.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "shared/Store.hpp"

using namespace Raul;

namespace Ingen {
namespace Events {


/** Note off with patch explicitly passed - triggered by MIDI.
 */
AllNotesOff::AllNotesOff(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, PatchImpl* patch)
: Event(engine, request, timestamp),
  _patch(patch)
{
}


/** Note off event with lookup - triggered by OSC.
 */
AllNotesOff::AllNotesOff(Engine& engine, SharedPtr<Request> request, SampleCount timestamp, const Path& patch_path)
: Event(engine, request, timestamp),
  _patch_path(patch_path),
  _patch(NULL)
{
}


void
AllNotesOff::execute(ProcessContext& context)
{
	Event::execute(context);

	if (!_patch)
		_patch = _engine.engine_store()->find_patch(_patch_path);

	//if (_patch != NULL)
	//	for (Raul::List<MidiInNode*>::iterator j = _patch->midi_in_nodes().begin(); j != _patch->midi_in_nodes().end(); ++j)
	//		(*j)->all_notes_off(offset);
}


void
AllNotesOff::post_process()
{
	if (_patch != NULL)
		_request->respond_ok();
}


} // namespace Ingen
} // namespace Events


