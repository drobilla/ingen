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

#ifndef INGEN_SHARED_URIS_HPP
#define INGEN_SHARED_URIS_HPP

#include <boost/utility.hpp>

#include "ingen/shared/LV2URIMap.hpp"
#include "ingen/shared/Forge.hpp"
#include "raul/Atom.hpp"
#include "raul/URI.hpp"

namespace Raul {
	class Forge;
}

namespace Ingen {
namespace Shared {

class URIs : public boost::noncopyable {
public:
	URIs(Ingen::Forge& forge, LV2URIMap* map);

	struct Quark : public Raul::URI {
		Quark(Ingen::Forge& forge, LV2URIMap* map, const char* str);
		operator LV2_URID()   const { return id; }
		operator Raul::Atom() const { return atom; }
		uint32_t   id;
		Raul::Atom atom;
	};

	Ingen::Forge& forge;

	const Quark atom_AtomPort;
	const Quark atom_Blank;
	const Quark atom_Bool;
	const Quark atom_Float;
	const Quark atom_Int;
	const Quark atom_Sequence;
	const Quark atom_Sound;
	const Quark atom_String;
	const Quark atom_URI;
	const Quark atom_URID;
	const Quark atom_Vector;
	const Quark atom_bufferType;
	const Quark atom_eventTransfer;
	const Quark atom_supports;
	const Quark doap_name;
	const Quark ingen_Connection;
	const Quark ingen_Internal;
	const Quark ingen_Node;
	const Quark ingen_Patch;
	const Quark ingen_Port;
	const Quark ingen_activity;
	const Quark ingen_broadcast;
	const Quark ingen_canvasX;
	const Quark ingen_canvasY;
	const Quark ingen_controlBinding;
	const Quark ingen_destination;
	const Quark ingen_document;
	const Quark ingen_enabled;
	const Quark ingen_engine;
	const Quark ingen_nil;
	const Quark ingen_node;
	const Quark ingen_polyphonic;
	const Quark ingen_polyphony;
	const Quark ingen_sampleRate;
	const Quark ingen_selected;
	const Quark ingen_source;
	const Quark ingen_uiEmbedded;
	const Quark ingen_value;
	const Quark lv2_AudioPort;
	const Quark lv2_CVPort;
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
	const Quark patch_Delete;
	const Quark patch_Get;
	const Quark patch_Move;
	const Quark patch_Patch;
	const Quark patch_Put;
	const Quark patch_Response;
	const Quark patch_Set;
	const Quark patch_add;
	const Quark patch_body;
	const Quark patch_destination;
	const Quark patch_remove;
	const Quark patch_request;
	const Quark patch_subject;
	const Quark rdf_instanceOf;
	const Quark rdf_type;
	const Quark rdfs_seeAlso;
	const Quark wildcard;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2URIMAP_HPP
