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

#define __STDC_LIMIT_MACROS 1

#include <assert.h>
#include <stdint.h>

#include <glib.h>

#include <boost/shared_ptr.hpp>

#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "raul/log.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

URIs::Quark::Quark(const char* c_str)
	: Raul::URI(c_str)
	, id(g_quark_from_string(c_str))
{
}

const char*
URIs::Quark::c_str() const
{
	return g_quark_to_string(id);
}

#define NS_CTX   "http://lv2plug.in/ns/ext/contexts#"
#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs(Raul::Forge& f)
	: forge(f)
	, atom_Bool             (LV2_ATOM__Bool)
	, atom_Float            (LV2_ATOM__Float)
	, atom_Int32            (LV2_ATOM__Int32)
	, atom_MessagePort      (LV2_ATOM__MessagePort)
	, atom_String           (LV2_ATOM__String)
	, atom_ValuePort        (LV2_ATOM__ValuePort)
	, atom_Vector           (LV2_ATOM__Vector)
	, atom_eventTransfer    (LV2_ATOM__eventTransfer)
	, atom_supports         (LV2_ATOM__supports)
	, ctx_audioContext      (NS_CTX "audioContext")
	, ctx_context           (NS_CTX "context")
	, ctx_messageContext    (NS_CTX "messageContext")
	, cv_CVPort             ("http://lv2plug.in/ns/ext/cv-port#CVPort")
	, doap_name             ("http://usefulinc.com/ns/doap#name")
	, ev_EventPort          ("http://lv2plug.in/ns/ext/event#EventPort")
	, ingen_Internal        (NS_INGEN "Internal")
	, ingen_Node            (NS_INGEN "Node")
	, ingen_Patch           (NS_INGEN "Patch")
	, ingen_Port            (NS_INGEN "Port")
	, ingen_broadcast       (NS_INGEN "broadcast")
	, ingen_controlBinding  (NS_INGEN "controlBinding")
	, ingen_document        (NS_INGEN "document")
	, ingen_enabled         (NS_INGEN "enabled")
	, ingen_engine          (NS_INGEN "engine")
	, ingen_nil             (NS_INGEN "nil")
	, ingen_node            (NS_INGEN "node")
	, ingen_polyphonic      (NS_INGEN "polyphonic")
	, ingen_polyphony       (NS_INGEN "polyphony")
	, ingen_sampleRate      (NS_INGEN "sampleRate")
	, ingen_selected        (NS_INGEN "selected")
	, ingen_value           (NS_INGEN "value")
	, ingen_canvasX         (NS_INGEN "canvasX")
	, ingen_canvasY         (NS_INGEN "canvasY")
	, lv2_AudioPort         (LV2_CORE__AudioPort)
	, lv2_ControlPort       (LV2_CORE__ControlPort)
	, lv2_InputPort         (LV2_CORE__InputPort)
	, lv2_OutputPort        (LV2_CORE__OutputPort)
	, lv2_Plugin            (LV2_CORE__Plugin)
	, lv2_connectionOptional(LV2_CORE__connectionOptional)
	, lv2_default           (LV2_CORE__default)
	, lv2_index             (LV2_CORE__index)
	, lv2_integer           (LV2_CORE__integer)
	, lv2_maximum           (LV2_CORE__maximum)
	, lv2_minimum           (LV2_CORE__minimum)
	, lv2_name              (LV2_CORE__name)
	, lv2_portProperty      (LV2_CORE__portProperty)
	, lv2_sampleRate        (LV2_CORE__sampleRate)
	, lv2_symbol            (LV2_CORE__symbol)
	, lv2_toggled           (LV2_CORE__toggled)
	, midi_Bender           (LV2_MIDI__Bender)
	, midi_ChannelPressure  (LV2_MIDI__ChannelPressure)
	, midi_Controller       (LV2_MIDI__Controller)
	, midi_MidiEvent        (LV2_MIDI__MidiEvent)
	, midi_NoteOn           (LV2_MIDI__NoteOn)
	, midi_controllerNumber (LV2_MIDI__controllerNumber)
	, midi_noteNumber       (LV2_MIDI__noteNumber)
	, rdf_instanceOf        (NS_RDF "instanceOf")
	, rdf_type              (NS_RDF "type")
	, rdfs_seeAlso          (NS_RDFS "seeAlso")
	, ui_Events             ("http://lv2plug.in/ns/extensions/ui#Events")
	, wildcard              (NS_INGEN "wildcard")
{
}

} // namespace Shared
} // namespace Ingen
