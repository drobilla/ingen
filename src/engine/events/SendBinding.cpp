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

#include <sstream>
#include "events/SendBinding.hpp"
#include "shared/LV2URIMap.hpp"
#include "Engine.hpp"
#include "PortImpl.hpp"
#include "ClientBroadcaster.hpp"

using namespace std;

namespace Ingen {
namespace Events {


void
SendBinding::post_process()
{
	const LV2URIMap& uris = *_engine.world()->uris().get();
	Raul::Atom::DictValue dict;
	if (_type == ControlBindings::MIDI_CC) {
		dict[uris.rdf_type] = uris.midi_Controller;
		dict[uris.midi_controllerNumber] = _num;
	} else if (_type == ControlBindings::MIDI_BENDER) {
		dict[uris.rdf_type] = uris.midi_Bender;
	} else if (_type == ControlBindings::MIDI_CHANNEL_PRESSURE) {
		dict[uris.rdf_type] = uris.midi_ChannelPressure;
	} else if (_type == ControlBindings::MIDI_NOTE) {
		dict[uris.rdf_type] = uris.midi_Note;
		dict[uris.midi_noteNumber] = _num;
	}
	// TODO: other event types
	_port->set_property(uris.ingen_controlBinding, dict); // FIXME: thread unsafe
	_engine.broadcaster()->set_property(_port->path(), uris.ingen_controlBinding, dict);
}


} // namespace Ingen
} // namespace Events

