/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_URIS_HPP
#define INGEN_SHARED_URIS_HPP

#include <boost/utility.hpp>

#include "ingen/shared/LV2URIMap.hpp"
#include "raul/URI.hpp"

namespace Raul {
	class Forge;
}

namespace Ingen {
namespace Shared {

class URIs : public boost::noncopyable {
public:
	URIs(Raul::Forge& forge, LV2URIMap* map);

	struct Quark : public Raul::URI {
		Quark(LV2URIMap* map, const char* str);
		uint32_t id;
	};

	Raul::Forge& forge;

	const Quark atom_Bool;
	const Quark atom_Float;
	const Quark atom_Int32;
	const Quark atom_MessagePort;
	const Quark atom_String;
	const Quark atom_ValuePort;
	const Quark atom_Vector;
	const Quark atom_eventTransfer;
	const Quark atom_supports;
	const Quark ctx_audioContext;
	const Quark ctx_context;
	const Quark ctx_messageContext;
	const Quark cv_CVPort;
	const Quark doap_name;
	const Quark ev_EventPort;
	const Quark ingen_Internal;
	const Quark ingen_Node;
	const Quark ingen_Patch;
	const Quark ingen_Port;
	const Quark ingen_activity;
	const Quark ingen_broadcast;
	const Quark ingen_controlBinding;
	const Quark ingen_document;
	const Quark ingen_enabled;
	const Quark ingen_engine;
	const Quark ingen_nil;
	const Quark ingen_node;
	const Quark ingen_polyphonic;
	const Quark ingen_polyphony;
	const Quark ingen_sampleRate;
	const Quark ingen_selected;
	const Quark ingen_value;
	const Quark ingen_canvasX;
	const Quark ingen_canvasY;
	const Quark lv2_AudioPort;
	const Quark lv2_ControlPort;
	const Quark lv2_InputPort;
	const Quark lv2_OutputPort;
	const Quark lv2_Plugin;
	const Quark lv2_connectionOptional;
	const Quark lv2_default;
	const Quark lv2_index;
	const Quark lv2_integer;
	const Quark lv2_maximum;
	const Quark lv2_minimum;
	const Quark lv2_name;
	const Quark lv2_portProperty;
	const Quark lv2_sampleRate;
	const Quark lv2_symbol;
	const Quark lv2_toggled;
	const Quark midi_Bender;
	const Quark midi_ChannelPressure;
	const Quark midi_Controller;
	const Quark midi_MidiEvent;
	const Quark midi_NoteOn;
	const Quark midi_controllerNumber;
	const Quark midi_noteNumber;
	const Quark rdf_instanceOf;
	const Quark rdf_type;
	const Quark rdfs_seeAlso;
	const Quark ui_Events;
	const Quark wildcard;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2URIMAP_HPP
