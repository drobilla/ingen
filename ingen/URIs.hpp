/*
  This file is part of Ingen.
  Copyright 2007-2013 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_URIS_HPP
#define INGEN_URIS_HPP

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Noncopyable.hpp"
#include "raul/URI.hpp"

namespace Raul {
class Forge;
}

namespace Ingen {

class URIMap;

/** Frequently used interned URIs.
 *
 * This class initially maps all the special URIs used throughout the code
 * using the URIMap so they can be used quickly with the performance of
 * integers, but still be dynamic.
 *
 * @ingroup ingen
 */
class INGEN_API URIs : public Raul::Noncopyable {
public:
	URIs(Ingen::Forge& forge, URIMap* map);

	struct Quark : public Raul::URI {
		Quark(Ingen::Forge& forge, URIMap* map, const char* str);
		operator LV2_URID() const { return id; }
		operator Atom()     const { return atom; }
		uint32_t id;
		Atom     atom;
	};

	Ingen::Forge& forge;

	const Quark atom_AtomPort;
	const Quark atom_Bool;
	const Quark atom_Chunk;
	const Quark atom_Float;
	const Quark atom_Int;
	const Quark atom_Object;
	const Quark atom_Sequence;
	const Quark atom_Sound;
	const Quark atom_String;
	const Quark atom_URI;
	const Quark atom_URID;
	const Quark atom_Vector;
	const Quark atom_bufferType;
	const Quark atom_eventTransfer;
	const Quark atom_supports;
	const Quark bufsz_maxBlockLength;
	const Quark bufsz_minBlockLength;
	const Quark bufsz_sequenceSize;
	const Quark doap_name;
	const Quark ingen_Arc;
	const Quark ingen_Block;
	const Quark ingen_Graph;
	const Quark ingen_GraphPrototype;
	const Quark ingen_Internal;
	const Quark ingen_activity;
	const Quark ingen_arc;
	const Quark ingen_block;
	const Quark ingen_broadcast;
	const Quark ingen_canvasX;
	const Quark ingen_canvasY;
	const Quark ingen_enabled;
	const Quark ingen_file;
	const Quark ingen_head;
	const Quark ingen_incidentTo;
	const Quark ingen_polyphonic;
	const Quark ingen_polyphony;
	const Quark ingen_prototype;
	const Quark ingen_sprungLayout;
	const Quark ingen_tail;
	const Quark ingen_uiEmbedded;
	const Quark ingen_value;
	const Quark log_Error;
	const Quark log_Note;
	const Quark log_Warning;
	const Quark lv2_AudioPort;
	const Quark lv2_CVPort;
	const Quark lv2_ControlPort;
	const Quark lv2_InputPort;
	const Quark lv2_OutputPort;
	const Quark lv2_Plugin;
	const Quark lv2_binary;
	const Quark lv2_connectionOptional;
	const Quark lv2_default;
	const Quark lv2_designation;
	const Quark lv2_extensionData;
	const Quark lv2_index;
	const Quark lv2_integer;
	const Quark lv2_maximum;
	const Quark lv2_minimum;
	const Quark lv2_name;
	const Quark lv2_port;
	const Quark lv2_portProperty;
	const Quark lv2_prototype;
	const Quark lv2_sampleRate;
	const Quark lv2_scalePoint;
	const Quark lv2_symbol;
	const Quark lv2_toggled;
	const Quark midi_Bender;
	const Quark midi_ChannelPressure;
	const Quark midi_Controller;
	const Quark midi_MidiEvent;
	const Quark midi_NoteOn;
	const Quark midi_binding;
	const Quark midi_controllerNumber;
	const Quark midi_noteNumber;
	const Quark morph_currentType;
	const Quark param_sampleRate;
	const Quark patch_Copy;
	const Quark patch_Delete;
	const Quark patch_Get;
	const Quark patch_Message;
	const Quark patch_Move;
	const Quark patch_Patch;
	const Quark patch_Put;
	const Quark patch_Response;
	const Quark patch_Set;
	const Quark patch_add;
	const Quark patch_body;
	const Quark patch_destination;
	const Quark patch_property;
	const Quark patch_remove;
	const Quark patch_request;
	const Quark patch_sequenceNumber;
	const Quark patch_subject;
	const Quark patch_value;
	const Quark patch_wildcard;
	const Quark pset_preset;
	const Quark pprops_logarithmic;
	const Quark rdf_type;
	const Quark rdfs_seeAlso;
	const Quark rsz_minimumSize;
	const Quark time_Position;
	const Quark time_bar;
	const Quark time_barBeat;
	const Quark time_beatUnit;
	const Quark time_beatsPerBar;
	const Quark time_beatsPerMinute;
	const Quark time_frame;
	const Quark time_speed;
};

} // namespace Ingen

#endif // INGEN_LV2URIMAP_HPP
