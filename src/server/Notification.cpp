/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

#include "ingen/shared/URIMap.hpp"
#include "ingen/shared/URIs.hpp"

#include "Broadcaster.hpp"
#include "Engine.hpp"
#include "Notification.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

void
Notification::post_process(Notification& note,
                           Engine&       engine)
{
	const Ingen::Shared::URIs& uris = engine.world()->uris();
	LV2_Atom_Forge             forge;
	uint8_t                    buf[128];
	switch (note.type) {
	case PORT_VALUE:
		engine.broadcaster()->set_property(note.port->path(),
		                                   uris.ingen_value,
		                                   note.value);
		break;
	case PORT_ACTIVITY:
		engine.broadcaster()->set_property(note.port->path(),
		                                   uris.ingen_activity,
		                                   note.value);
		break;
	case PORT_BINDING: {
		lv2_atom_forge_init(
			&forge, &engine.world()->uri_map().urid_map_feature()->urid_map);
		lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));
		LV2_Atom_Forge_Frame frame;
		switch (note.binding_type) {
		case ControlBindings::MIDI_CC:
			lv2_atom_forge_blank(&forge, &frame, 0, uris.midi_Controller);
			lv2_atom_forge_property_head(&forge, uris.midi_controllerNumber, 0);
			lv2_atom_forge_int(&forge, note.value.get_int32());
			break;
		case ControlBindings::MIDI_BENDER:
			lv2_atom_forge_blank(&forge, &frame, 0, uris.midi_Bender);
			break;
		case ControlBindings::MIDI_CHANNEL_PRESSURE:
			lv2_atom_forge_blank(&forge, &frame, 0, uris.midi_ChannelPressure);
			break;
		case ControlBindings::MIDI_NOTE:
			lv2_atom_forge_blank(&forge, &frame, 0, uris.midi_NoteOn);
			lv2_atom_forge_property_head(&forge, uris.midi_noteNumber, 0);
			lv2_atom_forge_int(&forge, note.value.get_int32());
			break;
		case ControlBindings::MIDI_RPN: // TODO
		case ControlBindings::MIDI_NRPN: // TODO
		case ControlBindings::NULL_CONTROL:
			break;
		}
		LV2_Atom* atom = (LV2_Atom*)buf;
		// FIXME: not thread-safe
		const Raul::Atom dict_atom = engine.world()->forge().alloc(
			atom->size, atom->type, LV2_ATOM_BODY(atom));
		note.port->set_property(uris.ingen_controlBinding, dict_atom);
		engine.broadcaster()->set_property(note.port->path(),
		                                   uris.ingen_controlBinding,
		                                   dict_atom);
		break;
	}
	case NIL:
		break;
	}
}

} // namespace Server
} // namespace Ingen
