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
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
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

#define NS_CTX     "http://lv2plug.in/ns/ext/contexts#"
#define NS_INGEN   "http://drobilla.net/ns/ingen#"
#define NS_LV2     "http://lv2plug.in/ns/lv2core#"
#define NS_MIDI    "http://drobilla.net/ns/ext/midi#"
#define NS_MIDI    "http://drobilla.net/ns/ext/midi#"
#define NS_RDF     "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS    "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs()
	: atom_AtomTransfer     (LV2_ATOM_URI "#AtomTransfer")
	, atom_Bool             (LV2_ATOM_URI "#Bool")
	, atom_Float32          (LV2_ATOM_URI "#Float32")
	, atom_Int32            (LV2_ATOM_URI "#Int32")
	, atom_MessagePort      (LV2_ATOM_URI "#MessagePort")
	, atom_String           (LV2_ATOM_URI "#String")
	, atom_ValuePort        (LV2_ATOM_URI "#ValuePort")
	, atom_Vector           (LV2_ATOM_URI "#Vector")
	, atom_supports         (LV2_ATOM_URI "#supports")
	, ctx_audioContext      (NS_CTX "audioContext")
	, ctx_context           (NS_CTX "context")
	, ctx_messageContext    (NS_CTX "messageContext")
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
	, ingen_canvas_x        (NS_INGEN "canvas-x")
	, ingen_canvas_y        (NS_INGEN "canvas-y")
	, lv2_AudioPort         (NS_LV2 "AudioPort")
	, lv2_ControlPort       (NS_LV2 "ControlPort")
	, lv2_InputPort         (NS_LV2 "InputPort")
	, lv2_OutputPort        (NS_LV2 "OutputPort")
	, lv2_Plugin            (NS_LV2 "Plugin")
	, lv2_connectionOptional(NS_LV2 "connectionOptional")
	, lv2_default           (NS_LV2 "default")
	, lv2_index             (NS_LV2 "index")
	, lv2_integer           (NS_LV2 "integer")
	, lv2_maximum           (NS_LV2 "maximum")
	, lv2_minimum           (NS_LV2 "minimum")
	, lv2_name              (NS_LV2 "name")
	, lv2_portProperty      (NS_LV2 "portProperty")
	, lv2_sampleRate        (NS_LV2 "sampleRate")
	, lv2_symbol            (NS_LV2 "symbol")
	, lv2_toggled           (NS_LV2 "toggled")
	, midi_Bender           (NS_MIDI "Bender")
	, midi_ChannelPressure  (NS_MIDI "ChannelPressure")
	, midi_Controller       (NS_MIDI "Controller")
	, midi_MidiEvent        ("http://lv2plug.in/ns/ext/midi#MidiEvent")
	, midi_Note             (NS_MIDI "Note")
	, midi_controllerNumber (NS_MIDI "controllerNumber")
	, midi_noteNumber       (NS_MIDI "noteNumber")
	, rdf_instanceOf        (NS_RDF "instanceOf")
	, rdf_type              (NS_RDF "type")
	, rdfs_seeAlso          (NS_RDFS "seeAlso")
	, ui_Events             ("http://lv2plug.in/ns/extensions/ui#Events")
	, wildcard              (NS_INGEN "wildcard")
{
}

uint32_t
URIs::map_uri(const char* uri)
{
	return static_cast<uint32_t>(g_quark_from_string(uri));
}

const char*
URIs::unmap_uri(uint32_t urid)
{
	return g_quark_to_string(urid);
}

} // namespace Shared
} // namespace Ingen
