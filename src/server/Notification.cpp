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

#include "ingen/shared/URIs.hpp"

#include "ClientBroadcaster.hpp"
#include "Engine.hpp"
#include "Notification.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

void
Notification::post_process(Notification& note,
                           Engine&       engine)
{
	const Ingen::Shared::URIs& uris  = engine.world()->uris();
	Ingen::Forge&              forge = engine.world()->forge();
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
		Raul::Atom::DictValue dict;
		switch (note.binding_type) {
		case ControlBindings::MIDI_CC:
			dict[uris.rdf_type]              = uris.midi_Controller;
			dict[uris.midi_controllerNumber] = note.value;
			break;
		case ControlBindings::MIDI_BENDER:
			dict[uris.rdf_type] = uris.midi_Bender;
			break;
		case ControlBindings::MIDI_CHANNEL_PRESSURE:
			dict[uris.rdf_type] = uris.midi_ChannelPressure;
			break;
		case ControlBindings::MIDI_NOTE:
			dict[uris.rdf_type]        = uris.midi_NoteOn;
			dict[uris.midi_noteNumber] = note.value;
			break;
		case ControlBindings::MIDI_RPN: // TODO
		case ControlBindings::MIDI_NRPN: // TODO
		case ControlBindings::NULL_CONTROL:
			break;
		}
		// FIXME: not thread-safe
		const Raul::Atom dict_atom = forge.alloc(dict);
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
