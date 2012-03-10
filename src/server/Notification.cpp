/* This file is part of Ingen.
 * Copyright 2011 David Robillard <http://drobilla.net>
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

#include "ingen/shared/LV2URIMap.hpp"

#include "ClientBroadcaster.hpp"
#include "Notification.hpp"
#include "PortImpl.hpp"

namespace Ingen {
namespace Server {

void
Notification::post_process(Notification& note,
                           Engine&       engine)
{
	switch (note.type) {
	case PORT_VALUE:
		engine.broadcaster()->set_property(
			note.port->path(),
			engine.world()->uris()->ingen_value, note.value);
		break;
	case PORT_ACTIVITY:
		engine.broadcaster()->activity(note.port->path(), note.value);
		break;
	case PORT_BINDING: {
		const Ingen::Shared::URIs& uris = *engine.world()->uris().get();
		Raul::Atom::DictValue dict;
		switch (note.binding_type) {
		case ControlBindings::MIDI_CC:
			dict[uris.rdf_type]              = uris.midi_Controller;
			dict[uris.midi_controllerNumber] = note.value.get_int32();
			break;
		case ControlBindings::MIDI_BENDER:
			dict[uris.rdf_type] = uris.midi_Bender;
			break;
		case ControlBindings::MIDI_CHANNEL_PRESSURE:
			dict[uris.rdf_type] = uris.midi_ChannelPressure;
			break;
		case ControlBindings::MIDI_NOTE:
			dict[uris.rdf_type]        = uris.midi_NoteOn;
			dict[uris.midi_noteNumber] = note.value.get_int32();
			break;
		case ControlBindings::MIDI_RPN: // TODO
		case ControlBindings::MIDI_NRPN: // TODO
		case ControlBindings::NULL_CONTROL:
			break;
		}
		note.port->set_property(uris.ingen_controlBinding, dict); // FIXME: thread unsafe
		engine.broadcaster()->set_property(note.port->path(),
		                                   uris.ingen_controlBinding,
		                                   dict);
		break;
	}
	case NIL:
		break;
	}
}

} // namespace Server
} // namespace Ingen
