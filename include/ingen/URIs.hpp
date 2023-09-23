/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

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
#include "ingen/URI.hpp"
#include "ingen/ingen.h"
#include "lilv/lilv.h"
#include "lv2/urid/urid.h"
#include "raul/Noncopyable.hpp"

namespace ingen {

class Forge;
class URIMap;

/** Frequently used interned URIs.
 *
 * This class initially maps all the special URIs used throughout the code
 * using the URIMap so they can be used quickly with the performance of
 * integers, but still be dynamic.
 *
 * @ingroup ingen
 */
class INGEN_API URIs : public raul::Noncopyable
{
public:
	URIs(ingen::Forge& ingen_forge, URIMap* map, LilvWorld* lworld);

	struct Quark : public URI {
		Quark(ingen::Forge& ingen_forge,
		      URIMap*       map,
		      LilvWorld*    lworld,
		      const char*   str);

		Quark(const Quark& copy);

		~Quark();

		const Atom& urid_atom() const { return _urid_atom; }
		const Atom& uri_atom() const { return _uri_atom; }

		LV2_URID        urid() const { return _urid_atom.get<LV2_URID>(); }
		const LilvNode* node() const { return _lilv_node; }

		operator LV2_URID()        const { return _urid_atom.get<LV2_URID>(); }
		explicit operator Atom()   const { return _urid_atom; }
		operator const LilvNode*() const { return _lilv_node; }

	private:
		Atom      _urid_atom;
		Atom      _uri_atom;
		LilvNode* _lilv_node;
	};

	ingen::Forge& forge;

	Quark atom_AtomPort;
	Quark atom_Bool;
	Quark atom_Chunk;
	Quark atom_Float;
	Quark atom_Int;
	Quark atom_Object;
	Quark atom_Path;
	Quark atom_Sequence;
	Quark atom_Sound;
	Quark atom_String;
	Quark atom_URI;
	Quark atom_URID;
	Quark atom_bufferType;
	Quark atom_eventTransfer;
	Quark atom_supports;
	Quark bufsz_maxBlockLength;
	Quark bufsz_minBlockLength;
	Quark bufsz_sequenceSize;
	Quark doap_name;
	Quark ingen_Arc;
	Quark ingen_Block;
	Quark ingen_BundleEnd;
	Quark ingen_BundleStart;
	Quark ingen_Graph;
	Quark ingen_GraphPrototype;
	Quark ingen_Internal;
	Quark ingen_Redo;
	Quark ingen_Undo;
	Quark ingen_activity;
	Quark ingen_arc;
	Quark ingen_block;
	Quark ingen_broadcast;
	Quark ingen_canvasX;
	Quark ingen_canvasY;
	Quark ingen_enabled;
	Quark ingen_externalContext;
	Quark ingen_file;
	Quark ingen_head;
	Quark ingen_incidentTo;
	Quark ingen_internalContext;
	Quark ingen_loadedBundle;
	Quark ingen_maxRunLoad;
	Quark ingen_meanRunLoad;
	Quark ingen_minRunLoad;
	Quark ingen_numThreads;
	Quark ingen_polyphonic;
	Quark ingen_polyphony;
	Quark ingen_prototype;
	Quark ingen_sprungLayout;
	Quark ingen_tail;
	Quark ingen_uiEmbedded;
	Quark ingen_value;
	Quark log_Error;
	Quark log_Note;
	Quark log_Trace;
	Quark log_Warning;
	Quark lv2_AudioPort;
	Quark lv2_CVPort;
	Quark lv2_ControlPort;
	Quark lv2_InputPort;
	Quark lv2_OutputPort;
	Quark lv2_Plugin;
	Quark lv2_appliesTo;
	Quark lv2_binary;
	Quark lv2_connectionOptional;
	Quark lv2_control;
	Quark lv2_default;
	Quark lv2_designation;
	Quark lv2_enumeration;
	Quark lv2_extensionData;
	Quark lv2_index;
	Quark lv2_integer;
	Quark lv2_maximum;
	Quark lv2_microVersion;
	Quark lv2_minimum;
	Quark lv2_minorVersion;
	Quark lv2_name;
	Quark lv2_port;
	Quark lv2_portProperty;
	Quark lv2_prototype;
	Quark lv2_sampleRate;
	Quark lv2_scalePoint;
	Quark lv2_symbol;
	Quark lv2_toggled;
	Quark midi_Bender;
	Quark midi_ChannelPressure;
	Quark midi_Controller;
	Quark midi_MidiEvent;
	Quark midi_NoteOn;
	Quark midi_binding;
	Quark midi_controllerNumber;
	Quark midi_noteNumber;
	Quark midi_channel;
	Quark morph_AutoMorphPort;
	Quark morph_MorphPort;
	Quark morph_currentType;
	Quark morph_supportsType;
	Quark opt_interface;
	Quark param_sampleRate;
	Quark patch_Copy;
	Quark patch_Delete;
	Quark patch_Get;
	Quark patch_Message;
	Quark patch_Move;
	Quark patch_Patch;
	Quark patch_Put;
	Quark patch_Response;
	Quark patch_Set;
	Quark patch_add;
	Quark patch_body;
	Quark patch_context;
	Quark patch_destination;
	Quark patch_property;
	Quark patch_remove;
	Quark patch_sequenceNumber;
	Quark patch_subject;
	Quark patch_value;
	Quark patch_wildcard;
	Quark pprops_logarithmic;
	Quark pset_Preset;
	Quark pset_preset;
	Quark rdf_type;
	Quark rdfs_Class;
	Quark rdfs_label;
	Quark rdfs_seeAlso;
	Quark rsz_minimumSize;
	Quark state_loadDefaultState;
	Quark state_state;
	Quark time_Position;
	Quark time_bar;
	Quark time_barBeat;
	Quark time_beatUnit;
	Quark time_beatsPerBar;
	Quark time_beatsPerMinute;
	Quark time_frame;
	Quark time_speed;
	Quark work_schedule;
};

inline bool
operator==(const URIs::Quark& lhs, const Atom& rhs)
{
	if (rhs.type() == lhs.urid_atom().type()) {
		return rhs == lhs.urid_atom();
	}

	if (rhs.type() == lhs.uri_atom().type()) {
		return rhs == lhs.uri_atom();
	}

	return false;
}

inline bool
operator==(const Atom& lhs, const URIs::Quark& rhs)
{
	return rhs == lhs;
}

inline bool
operator!=(const Atom& lhs, const URIs::Quark& rhs)
{
	return !(lhs == rhs);
}

inline bool
operator!=(const URIs::Quark& lhs, const Atom& rhs)
{
	return !(lhs == rhs);
}

} // namespace ingen

#endif // INGEN_URIS_HPP
