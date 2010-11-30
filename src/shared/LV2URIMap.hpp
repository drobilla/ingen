/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_LV2URIMAP_HPP
#define INGEN_SHARED_LV2URIMAP_HPP

#include <boost/utility.hpp>
#include "raul/URI.hpp"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/uri-unmap/uri-unmap.h"
#include "ingen-config.h"
#include "LV2Features.hpp"

namespace Ingen {
namespace Shared {


/** Implementation of the LV2 URI Map and URI Unmap extensions
 */
class LV2URIMap : public boost::noncopyable, public LV2Features::Feature {
public:
	LV2URIMap();

	SharedPtr<LV2_Feature> feature(Node*) {
		return SharedPtr<LV2_Feature>(&uri_map_feature, NullDeleter<LV2_Feature>);
	}

	struct UnmapFeature : public LV2Features::Feature {
		UnmapFeature(const LV2URIMap& map) : _feature(map.uri_unmap_feature) {}
		
		SharedPtr<LV2_Feature> feature(Node*) {
			return SharedPtr<LV2_Feature>(&_feature, NullDeleter<LV2_Feature>);
		}

		LV2_Feature _feature;
	};

	SharedPtr<UnmapFeature> unmap_feature() { return _unmap_feature; }
		
	virtual uint32_t    uri_to_id(const char* map, const char* uri);
	virtual const char* id_to_uri(const char* map, uint32_t id);

private:
	static uint32_t uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
	                                  const char*               map,
	                                  const char*               uri);

	static const char* uri_unmap_id_to_uri(LV2_URI_Map_Callback_Data callback_data,
	                                       const char*               map,
	                                       const uint32_t            id);

	LV2_Feature         uri_map_feature;
	LV2_URI_Map_Feature uri_map_feature_data;

	LV2_Feature           uri_unmap_feature;
	LV2_URI_Unmap_Feature uri_unmap_feature_data;

	SharedPtr<UnmapFeature> _unmap_feature;
	
public:
	struct Quark : public Raul::URI {
		Quark(const char* str);
		const char* c_str() const;
		uint32_t id;
	};

	const Quark atom_AtomTransfer;
	const Quark atom_Bool;
	const Quark atom_Float32;
	const Quark atom_Int32;
	const Quark atom_MessagePort;
	const Quark atom_String;
	const Quark atom_ValuePort;
	const Quark atom_Vector;
	const Quark atom_supports;
	const Quark ctx_AudioContext;
	const Quark ctx_MessageContext;
	const Quark ctx_context;
	const Quark doap_name;
	const Quark ev_EventPort;
	const Quark ingen_Internal;
	const Quark ingen_LADSPAPlugin;
	const Quark ingen_Node;
	const Quark ingen_Patch;
	const Quark ingen_Port;
	const Quark ingen_broadcast;
	const Quark ingen_controlBinding;
	const Quark ingen_document;
	const Quark ingen_enabled;
	const Quark ingen_nil;
	const Quark ingen_node;
	const Quark ingen_polyphonic;
	const Quark ingen_polyphony;
	const Quark ingen_selected;
	const Quark ingen_value;
	const Quark ingenui_canvas_x;
	const Quark ingenui_canvas_y;
	const Quark lv2_AudioPort;
	const Quark lv2_ControlPort;
	const Quark lv2_InputPort;
	const Quark lv2_OutputPort;
	const Quark lv2_Plugin;
	const Quark lv2_default;
	const Quark lv2_index;
	const Quark lv2_integer;
	const Quark lv2_maximum;
	const Quark lv2_minimum;
	const Quark lv2_name;
	const Quark lv2_portProperty;
	const Quark lv2_symbol;
	const Quark lv2_toggled;
	const Quark midi_Bender;
	const Quark midi_ChannelPressure;
	const Quark midi_Controller;
	const Quark midi_MidiEvent;
	const Quark midi_Note;
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
